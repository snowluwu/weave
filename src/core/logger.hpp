#pragma once

#include "pch.h"

namespace weave {

    enum class Severity {
        Trace = 0,
        Debug = 1,
        Info = 2,
        Warn = 3,
        Error = 4,
        Off = 5
    };

    const char* LevelToString(Severity level);

    namespace Logger {

        bool Init();
        // drains the queue; call only outside dllmain
        void Shutdown();
        bool IsWriterThread();
        void Host(Severity level, const char* message);
        void SetLevel(Severity level);
        bool Enabled(Severity level);

        // preserve the intercepted api error
        struct ErrorScope {
            DWORD error = GetLastError();

            ~ErrorScope() {
                SetLastError(error);
            }
        };

        void Write(Severity level, const char* file, int line, const char* fmt, ...);

    }

}

#define WEAVE_LOG(level, fmt, ...)                                                                                    \
    do {                                                                                                               \
        if (weave::Logger::Enabled(level)) {                                                                          \
            weave::Logger::ErrorScope errorScope;                                                                     \
            weave::Logger::Write(level, __FILE__, __LINE__, fmt, ##__VA_ARGS__);                                      \
        }                                                                                                              \
    } while (0)
#define LOG_TRACE(fmt, ...) WEAVE_LOG(weave::Severity::Trace, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) WEAVE_LOG(weave::Severity::Debug, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) WEAVE_LOG(weave::Severity::Info, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) WEAVE_LOG(weave::Severity::Warn, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) WEAVE_LOG(weave::Severity::Error, fmt, ##__VA_ARGS__)
