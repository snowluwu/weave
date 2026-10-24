#include "pch.h"
#include "weave.h"
#include "version.h"
#include "src/core/runtime.hpp"
#include "src/core/config.hpp"
#include "src/core/logger.hpp"
#include "src/hook/hooks.hpp"
#include "src/hook/udp.hpp"
#include "src/core/utils.hpp"

namespace weave {
    static std::mutex g_lifecycle, g_gate;
    static bool g_configured = false, g_running = false, g_installed = false;
    static uint64_t g_calls = 0;
    static std::atomic<uint64_t> g_work{0};
    static HANDLE g_stopEvent = nullptr, g_worker = nullptr;
    static thread_local unsigned g_depth = 0;

    CallScope::CallScope() {
        Logger::ErrorScope preserve;
        std::lock_guard<std::mutex> lock(g_gate);
        entered = g_running;
        if (entered) {
            ++g_calls;
            ++g_depth;
        }
    }

    CallScope::~CallScope() {
        Logger::ErrorScope preserve;
        if (entered) {
            std::lock_guard<std::mutex> lock(g_gate);
            --g_calls;
            --g_depth;
        }
    }

    WorkScope::WorkScope() { ++g_work; }
    WorkScope::~WorkScope() { --g_work; }

    struct Work {
        PTP_SIMPLE_CALLBACK callback;
        void* context;
    };
    static void CALLBACK RunWork(PTP_CALLBACK_INSTANCE instance, void* context) {
        std::unique_ptr<Work> work(static_cast<Work*>(context));
        try {
            work->callback(instance, work->context);
        } catch (...) {
            LOG_ERROR("background callback failed");
        }
        --g_work;
    }

    bool SubmitWork(PTP_SIMPLE_CALLBACK callback, void* context, PTP_CALLBACK_ENVIRON environment) {
        auto work = std::make_unique<Work>(Work{callback, context});
        ++g_work;
        if (!TrySubmitThreadpoolCallback(RunWork, work.get(), environment)) {
            --g_work;
            return false;
        }
        work.release();
        return true;
    }

    static void ReconnectWorker() {
        while (WaitForSingleObject(g_stopEvent, g_cfg.reconnectInterval) == WAIT_TIMEOUT) {
            std::vector<UdpFlow> sockets;
            {
                std::shared_lock<std::shared_mutex> lock(g_stateMutex);
                for (const auto& entry : g_udpMap)
                    sockets.push_back({entry.first, entry.second.flowId});
            }
            for (const auto& flow : sockets) {
                SOCKET closedControl = INVALID_SOCKET;
                UdpAssociation summary;
                bool writeSummary = false;
                bool pending = false;
                int error = 0;
                const char* reason = "connected";
                {
                    std::lock_guard<std::shared_mutex> lock(g_stateMutex);
                    auto it = g_udpMap.find(flow.socket);
                    if (it == g_udpMap.end() || it->second.flowId != flow.flowId)
                        continue;
                    auto& association = it->second;
                    if (association.status == UdpAssociation::Status::Associated &&
                        IsSocketDisconnected(association.ctrlSocket, &error, &reason)) {
                        closedControl = association.ctrlSocket;
                        association.ctrlSocket = INVALID_SOCKET;
                        association.status = UdpAssociation::Status::PendingAssociate;
                    }
                    pending = association.status == UdpAssociation::Status::PendingAssociate;
                    const uint64_t now = GetTickCount64();
                    if (Logger::Enabled(Severity::Debug) && association.status != UdpAssociation::Status::Idle &&
                        now - association.lastSummaryAt >= 10000) {
                        summary = association;
                        association.reportedTx = association.txPackets;
                        association.reportedRx = association.rxPackets;
                        association.lastSummaryAt = now;
                        writeSummary = true;
                    }
                }
                if (closedControl != INVALID_SOCKET) {
                    Hooks::Real_closesocket(closedControl);
                    LOG_WARN("event=udp.associate.pending flow=%llu socket=%llu trigger=worker reason=%s error=%d",
                             flow.flowId, (ULONG_PTR)flow.socket, reason, error);
                }
                if (writeSummary)
                    LogUdpSummary(flow.socket, summary, "udp.summary");
                if (pending)
                    QueueUdpAssociation(flow.socket);
            }
        }
        LOG_TRACE("reconnect worker exiting");
    }

    static DWORD WINAPI WorkerMain(void*) {
        try {
            ReconnectWorker();
        } catch (...) {
            LOG_ERROR("UDP reconnect worker stopped unexpectedly");
        }
        return 0;
    }


    static WeaveResult Configure(const WeaveOptions* options) {
        if (Logger::IsWriterThread() || g_depth)
            return WEAVE_BUSY;
        std::lock_guard<std::mutex> lifecycle(g_lifecycle);
        {
            std::lock_guard<std::mutex> gate(g_gate);
            if (g_running)
                return WEAVE_BUSY;
        }
        if (!options || options->size != sizeof(WeaveOptions))
            return WEAVE_INVALID_ARGUMENT;
        if (options->api_version != WEAVE_API_VERSION)
            return WEAVE_INCOMPATIBLE_API;
        if (!options->host || !*options->host || !options->port ||
            options->connect_timeout_ms < 100 || options->connect_timeout_ms > INT_MAX ||
            options->reconnect_interval_ms < 100 || options->reconnect_interval_ms > INT_MAX ||
            options->log_level > WEAVE_LOG_OFF)
            return WEAVE_INVALID_ARGUMENT;
        Config config;
        config.host = options->host;
        config.port = options->port;
        config.user = options->username ? options->username : "";
        config.password = options->password ? options->password : "";
        if (config.user.empty() != config.password.empty() || config.user.size() > 255 || config.password.size() > 255)
            return WEAVE_INVALID_ARGUMENT;
        config.connectTimeout = options->connect_timeout_ms;
        config.reconnectInterval = options->reconnect_interval_ms;
        config.logLevel = static_cast<Severity>(options->log_level);
        config.logCallback = options->log_callback;
        config.logContext = options->log_context;
        g_cfg = std::move(config);
        ResetProxyAddress();
        g_configured = true;
        return WEAVE_OK;
    }

    static WeaveResult StartEngine() {
        if (Logger::IsWriterThread() || g_depth)
            return WEAVE_BUSY;
        std::lock_guard<std::mutex> lifecycle(g_lifecycle);
        if (!g_configured)
            return WEAVE_NOT_CONFIGURED;
        {
            std::lock_guard<std::mutex> gate(g_gate);
            if (g_running)
                return WEAVE_OK;
        }
        HMODULE pinned;
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                               reinterpret_cast<LPCWSTR>(&WeaveStart), &pinned))
            return WEAVE_START_FAILED;
        Logger::SetLevel(g_cfg.logLevel);
        if (!Logger::Init())
            return WEAVE_START_FAILED;
        g_stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (g_stopEvent)
            g_worker = CreateThread(nullptr, 0, WorkerMain, nullptr, 0, nullptr);
        if (!g_worker) {
            const DWORD error = GetLastError();
            if (g_stopEvent)
                CloseHandle(g_stopEvent);
            g_stopEvent = nullptr;
            Logger::Shutdown();
            SetLastError(error);
            return WEAVE_START_FAILED;
        }
        if (!g_installed && !Hooks::Install()) {
            SetEvent(g_stopEvent);
            WaitForSingleObject(g_worker, INFINITE);
            CloseHandle(g_worker);
            CloseHandle(g_stopEvent);
            g_worker = g_stopEvent = nullptr;
            Logger::Shutdown();
            SetLastError(ERROR_DLL_INIT_FAILED);
            return WEAVE_START_FAILED;
        }
        g_installed = true;
        {
            std::lock_guard<std::mutex> gate(g_gate);
            g_running = true;
        }
        LOG_INFO("event=weave.start version=%s", WEAVE_VERSION);
        return WEAVE_OK;
    }

    static WeaveResult StopEngine() {
        if (Logger::IsWriterThread() || g_depth)
            return WEAVE_BUSY;
        std::lock_guard<std::mutex> lifecycle(g_lifecycle);
        {
            std::lock_guard<std::mutex> gate(g_gate);
            if (!g_running)
                return WEAVE_OK;
            if (g_calls || g_work.load())
                return WEAVE_BUSY;
            std::shared_lock<std::shared_mutex> state(g_stateMutex);
            if (!g_udpMap.empty() || !g_tcpConnections.empty() || HasUdpIo() || HasAsyncConnections())
                return WEAVE_BUSY;
            g_running = false;
        }
        SetEvent(g_stopEvent);
        WaitForSingleObject(g_worker, INFINITE);
        CloseHandle(g_worker);
        CloseHandle(g_stopEvent);
        g_worker = g_stopEvent = nullptr;
        LOG_INFO("event=weave.stop");
        Logger::Shutdown();
        return WEAVE_OK;
    }
}

extern "C" {
    WeaveResult __cdecl WeaveConfigure(const WeaveOptions* options) {
        try { return weave::Configure(options); }
        catch (...) { SetLastError(ERROR_NOT_ENOUGH_MEMORY); return WEAVE_INVALID_ARGUMENT; }
    }
    WeaveResult __cdecl WeaveStart() {
        try { return weave::StartEngine(); }
        catch (...) { SetLastError(ERROR_NOT_ENOUGH_MEMORY); return WEAVE_START_FAILED; }
    }
    WeaveResult __cdecl WeaveStop() {
        try { return weave::StopEngine(); }
        catch (...) { SetLastError(ERROR_INVALID_STATE); return WEAVE_STOP_FAILED; }
    }
    WeaveResult __cdecl WeaveSetLogLevel(uint32_t level) {
        if (level > WEAVE_LOG_OFF) return WEAVE_INVALID_ARGUMENT;
        weave::Logger::SetLevel(static_cast<weave::Severity>(level));
        return WEAVE_OK;
    }
    const char* __cdecl WeaveGetVersion() { return WEAVE_VERSION; }
    void __cdecl WeaveLog(uint32_t level, const char* message) {
        if (message && level < WEAVE_LOG_OFF)
            weave::Logger::Host(static_cast<weave::Severity>(level), message);
    }
}
