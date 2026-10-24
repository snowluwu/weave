#pragma once
#include <stdint.h>

#ifdef WEAVE_PROXY_EXPORTS
#define WEAVE_API __declspec(dllexport)
#else
#define WEAVE_API __declspec(dllimport)
#endif

#define WEAVE_API_VERSION 1u

#ifdef __cplusplus
extern "C" {
#endif

typedef enum WeaveResult {
    WEAVE_OK = 0,
    WEAVE_INVALID_ARGUMENT = 1,
    WEAVE_INCOMPATIBLE_API = 2,
    WEAVE_NOT_CONFIGURED = 3,
    WEAVE_BUSY = 4,
    WEAVE_START_FAILED = 5,
    WEAVE_STOP_FAILED = 6
} WeaveResult;

typedef enum WeaveLogLevel {
    WEAVE_LOG_TRACE = 0,
    WEAVE_LOG_DEBUG = 1,
    WEAVE_LOG_INFO = 2,
    WEAVE_LOG_WARN = 3,
    WEAVE_LOG_ERROR = 4,
    WEAVE_LOG_OFF = 5
} WeaveLogLevel;

typedef struct WeaveLogRecord {
    uint32_t size;
    uint32_t level;
    uint32_t process_id;
    uint32_t thread_id;
    uint64_t timestamp_utc_100ns;
    const char* source;
    const char* file;
    uint32_t line;
    const char* message;
} WeaveLogRecord;

typedef void (__cdecl *WeaveLogCallback)(const WeaveLogRecord* record, void* context);

typedef struct WeaveOptions {
    uint32_t size;
    uint32_t api_version;
    const char* host;
    uint16_t port;
    const char* username;
    const char* password;
    uint32_t connect_timeout_ms;
    uint32_t reconnect_interval_ms;
    uint32_t log_level;
    WeaveLogCallback log_callback;
    void* log_context;
} WeaveOptions;

WEAVE_API WeaveResult __cdecl WeaveConfigure(const WeaveOptions* options);
WEAVE_API WeaveResult __cdecl WeaveStart(void);
WEAVE_API WeaveResult __cdecl WeaveStop(void);
WEAVE_API WeaveResult __cdecl WeaveSetLogLevel(uint32_t level);
WEAVE_API const char* __cdecl WeaveGetVersion(void);
WEAVE_API void __cdecl WeaveLog(uint32_t level, const char* message);

#ifdef __cplusplus
}

namespace weave {
    using Result = ::WeaveResult;
    using Options = ::WeaveOptions;
    using LogRecord = ::WeaveLogRecord;
    using LogCallback = ::WeaveLogCallback;

    enum class LogLevel : uint32_t {
        Trace = WEAVE_LOG_TRACE,
        Debug = WEAVE_LOG_DEBUG,
        Info = WEAVE_LOG_INFO,
        Warn = WEAVE_LOG_WARN,
        Error = WEAVE_LOG_ERROR,
        Off = WEAVE_LOG_OFF
    };

    inline Result Configure(const Options& options) { return ::WeaveConfigure(&options); }
    inline Result Start() { return ::WeaveStart(); }
    inline Result Stop() { return ::WeaveStop(); }
    inline Result SetLogLevel(LogLevel level) { return ::WeaveSetLogLevel(static_cast<uint32_t>(level)); }
    inline const char* GetVersion() { return ::WeaveGetVersion(); }
    inline void WriteLog(LogLevel level, const char* message) { ::WeaveLog(static_cast<uint32_t>(level), message); }
}
#endif
