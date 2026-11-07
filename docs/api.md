# API Reference

`include/weave.h` declares a C ABI using `__cdecl` and C++ wrappers in `namespace weave`. The exported names use the `Weave` prefix; wrappers call the same exports. All non-null pointers passed to the API must point to valid data for the duration of the call. Text parameters and callback strings are null-terminated byte strings; weave does not perform text encoding conversion.

## Options

Initialize `WeaveOptions` (C) or `weave::Options` (C++) to zero and set these fields:

| Field | Meaning |
| --- | --- |
| `size` | Must equal `sizeof(WeaveOptions)`. |
| `api_version` | Must equal `WEAVE_API_VERSION`. |
| `host` | Nonempty, null-terminated proxy hostname or IPv4 address. |
| `port` | Proxy TCP port, 1–65535. |
| `username`, `password` | Both null/empty for no credentials, or both nonempty and at most 255 bytes each. |
| `connect_timeout_ms` | SOCKS connection timeout in milliseconds, 100 through `INT_MAX`. |
| `reconnect_interval_ms` | UDP association retry interval in milliseconds, 100 through `INT_MAX`. |
| `log_level` | One of `WEAVE_LOG_TRACE` through `WEAVE_LOG_OFF`. |
| `log_callback` | Optional `WeaveLogCallback`; null disables delivery. |
| `log_context` | Opaque pointer passed to the callback; may be null. |

Strings are copied during successful configuration. Callback code and context remain owned by the host and must stay valid until `Stop` succeeds. `Configure` validates fields but does not connect to the proxy or verify credentials. A rejected configuration leaves the previous one intact.

## Functions

| C export | C++ wrapper | Behavior |
| --- | --- | --- |
| `WeaveConfigure(const WeaveOptions*)` | `weave::Configure(const weave::Options&)` | Copies options while stopped; does not enable interception. |
| `WeaveStart()` | `weave::Start()` | Starts the configured engine; returns `WEAVE_OK` if already running. |
| `WeaveStop()` | `weave::Stop()` | Stops when no managed sockets or pending work remain; returns `WEAVE_OK` if already stopped. |
| `WeaveSetLogLevel(uint32_t)` | `weave::SetLogLevel(weave::LogLevel)` | Changes the current logging threshold. A later `Start` restores the configured threshold. |
| `WeaveGetVersion()` | `weave::GetVersion()` | Returns a library-owned version string, such as `0.0.1+abcdef123456`. Do not free it. |
| `WeaveLog(uint32_t, const char*)` | `weave::WriteLog(weave::LogLevel, const char*)` | Queues a host message when logging is active and the level is enabled; null messages and invalid/off levels are ignored. |

`Configure`, `Start`, `Stop` and `SetLogLevel` return `WeaveResult`. Call lifecycle functions outside loader-lock contexts. For threading and stopping rules, see [Lifecycle](lifecycle.md).

## Results

| Value | Meaning |
| --- | --- |
| `WEAVE_OK` | Operation succeeded. |
| `WEAVE_INVALID_ARGUMENT` | Invalid options or log level; configuration can also return this after an allocation failure. |
| `WEAVE_INCOMPATIBLE_API` | `api_version` does not match. |
| `WEAVE_NOT_CONFIGURED` | Start was called before a successful configuration. |
| `WEAVE_BUSY` | The engine is running during configuration, an operation prevents stopping, or a lifecycle function was called from an intercepted call or log callback. |
| `WEAVE_START_FAILED` | Initialization failed. |
| `WEAVE_STOP_FAILED` | Shutdown failed unexpectedly. |

`GetLastError()` may contain additional Windows error information after startup or shutdown failure; read it immediately. It is not a replacement for the result code and is not guaranteed to identify every failure.

## Types and Log Levels

C++ aliases `weave::Result`, `weave::Options`, `weave::LogRecord` and `weave::LogCallback` map to their C types. `weave::LogLevel` maps `Trace`, `Debug`, `Info`, `Warn`, `Error` and `Off` to the corresponding `WEAVE_LOG_*` values. Lower values are more detailed; `Off` disables ordinary records. See [Logging](logging.md) for record fields and callback rules.
