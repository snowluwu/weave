# Logging

## Receive Records

Set `log_callback` and optionally `log_context` in `WeaveOptions`. With no callback, records are not delivered and weave does not create a log file. The host chooses the storage, formatting and retention policy.

```cpp
#include <cstdio>
#include <weave.h>

void __cdecl OnLog(const weave::LogRecord* record, void* context) {
    auto* output = static_cast<FILE*>(context);
    std::fprintf(output, "[%u] %s: %s\n", record->level, record->source, record->message);
}

// before Configure:
// options.log_callback = OnLog;
// options.log_context = stderr;
```

A dedicated writer thread invokes the callback outside networking locks. The record and its string pointers are valid only for the callback; copy anything needed later. Keep the callback, its module and context alive until `Stop` succeeds. Keep the callback short; do not wait for network threads, call lifecycle functions, or allow exceptions to cross its boundary.

## Record Fields

| Field | Meaning |
| --- | --- |
| `size` | Size of `WeaveLogRecord`. |
| `level` | A `WEAVE_LOG_*` severity. |
| `process_id`, `thread_id` | Process and producer thread identifiers. |
| `timestamp_utc_100ns` | UTC time in Windows FILETIME units: 100 ns intervals since 1601-01-01 UTC. |
| `source` | `weave` for library messages, `host` for messages submitted by the application. |
| `file`, `line` | Source location for library messages; host messages use an empty filename and line 0. |
| `message` | Null-terminated message text. |

## Levels and Queue

Levels in order are `TRACE`, `DEBUG`, `INFO`, `WARN`, `ERROR`, then `OFF`. Configure a threshold in `log_level` or change it with `SetLogLevel`; the next start resets it to the configured value. Use `WriteLog` to submit host messages at a chosen level. Calls from the callback itself are suppressed to avoid recursion.

The pending queue holds up to 1024 records. Once full, new records of any level are dropped; the writer reports the number dropped in a later warning record. A slow callback can delay delivery and shutdown. Logging is best-effort: allocation and writer failures are reported through Windows debugger output where possible.
