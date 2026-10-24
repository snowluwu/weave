#include "pch.h"
#include "src/core/logger.hpp"
#include "src/core/config.hpp"
#include <cstdarg>

namespace weave {
    struct Record {
        FILETIME time{};
        DWORD thread = 0;
        Severity level = Severity::Info;
        std::string source, file, message;
        uint32_t line = 0;
    };

    static std::mutex g_mutex;
    static std::deque<Record> g_queue;
    static HANDLE g_event = nullptr, g_thread = nullptr;
    static std::atomic<DWORD> g_writerId{0};
    static bool g_accepting = false;
    static uint64_t g_dropped = 0;
    static std::atomic<Severity> g_level{Severity::Info};
    static constexpr size_t QueueCapacity = 1024;

    const char* LevelToString(Severity level) {
        static const char* names[] = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "OFF"};
        return names[static_cast<unsigned>(level)];
    }

    static void Deliver(const Record& record) {
        const WeaveLogRecord view{sizeof(WeaveLogRecord), static_cast<uint32_t>(record.level),
            GetCurrentProcessId(), record.thread,
            (static_cast<uint64_t>(record.time.dwHighDateTime) << 32) | record.time.dwLowDateTime,
            record.source.c_str(), record.file.c_str(), record.line, record.message.c_str()};
        try {
            g_cfg.logCallback(&view, g_cfg.logContext);
        } catch (...) {
            OutputDebugStringA("weave: log callback threw an exception\n");
        }
    }

    static DWORD WINAPI Writer(void*) {
        g_writerId.store(GetCurrentThreadId());
        try {
            for (;;) {
                std::deque<Record> batch;
                bool stopping;
                uint64_t dropped;
                {
                    std::lock_guard<std::mutex> lock(g_mutex);
                    batch.swap(g_queue);
                    stopping = !g_accepting;
                    dropped = g_dropped;
                    g_dropped = 0;
                }
                for (const auto& record : batch)
                    Deliver(record);
                if (dropped) {
                    Record report;
                    GetSystemTimeAsFileTime(&report.time);
                    report.thread = GetCurrentThreadId();
                    report.level = Severity::Warn;
                    report.source = "weave";
                    report.message = "event=logger.dropped count=" + std::to_string(dropped);
                    Deliver(report);
                }
                if (stopping)
                    return 0;
                WaitForSingleObject(g_event, INFINITE);
            }
        } catch (...) {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_accepting = false;
            g_queue.clear();
            OutputDebugStringA("weave: log writer failed\n");
            return 0;
        }
    }

    bool Logger::Init() {
        if (!g_cfg.logCallback)
            return true;
        g_event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!g_event)
            return false;
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_accepting = true;
        }
        g_thread = CreateThread(nullptr, 0, Writer, nullptr, 0, nullptr);
        if (!g_thread) {
            const DWORD error = GetLastError();
            std::lock_guard<std::mutex> lock(g_mutex);
            g_accepting = false;
            CloseHandle(g_event);
            g_event = nullptr;
            SetLastError(error);
            return false;
        }
        return true;
    }

    bool Logger::IsWriterThread() {
        return g_writerId.load() != 0 && g_writerId.load() == GetCurrentThreadId();
    }

    void Logger::Shutdown() {
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_accepting = false;
            if (g_event)
                SetEvent(g_event);
        }
        if (g_thread) {
            WaitForSingleObject(g_thread, INFINITE);
            CloseHandle(g_thread);
            CloseHandle(g_event);
            g_thread = g_event = nullptr;
            g_writerId = 0;
        }
    }

    void Logger::SetLevel(Severity level) { g_level.store(level); }
    bool Logger::Enabled(Severity level) { return level >= g_level.load() && level != Severity::Off; }

    static void Enqueue(Severity level, const char* source, const char* file, int line, const char* message) {
        if (!Logger::Enabled(level) || Logger::IsWriterThread())
            return;
        Logger::ErrorScope preserve;
        try {
            std::lock_guard<std::mutex> lock(g_mutex);
            if (!g_accepting)
                return;
            if (g_queue.size() >= QueueCapacity) {
                ++g_dropped;
                return;
            }
            Record record;
            GetSystemTimeAsFileTime(&record.time);
            record.thread = GetCurrentThreadId();
            record.level = level;
            record.source = source;
            record.file = file;
            record.line = line;
            record.message = message;
            g_queue.push_back(std::move(record));
            SetEvent(g_event);
        } catch (...) {
            OutputDebugStringA("weave: unable to queue log record\n");
        }
    }

    void Logger::Host(Severity level, const char* message) { Enqueue(level, "host", "", 0, message); }

    void Logger::Write(Severity level, const char* file, int line, const char* fmt, ...) {
        ErrorScope preserve;
        char message[2500];
        va_list args;
        va_start(args, fmt);
        vsnprintf_s(message, sizeof(message), _TRUNCATE, fmt, args);
        va_end(args);
        const char* name = strrchr(file, '\\');
        if (!name)
            name = strrchr(file, '/');
        Enqueue(level, "weave", name ? name + 1 : file, line, message);
    }
}
