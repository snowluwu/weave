# weave

Windows x64 SOCKS5 interception library, extracted from [droute](https://github.com/snowluwu/droute).

`weave_proxy.dll` owns Winsock hooks, TCP and UDP proxy connections, asynchronous I/O and reconnect handling. The host owns configuration, application updates and log storage. Loading the DLL does not enable interception.

## build

Clone with `--recurse-submodules`. Use Visual Studio 2022 with the C++ desktop workload and Windows SDK:

```powershell
msbuild weave.sln /p:Configuration=Release /p:Platform=x64
```

Outputs are in `dist/x64/Release`. Link `weave_proxy.lib`, include `include/weave.h`, and ship `weave_proxy.dll` beside the executable. Keep the matching PDB for debugging.

## use

```cpp
#include <weave.h>

void __cdecl WriteLog(const weave::LogRecord* record, void* context) {
    // copy fields here if they need to outlive this callback
}

weave::Options options{};
options.size = sizeof(options);
options.api_version = WEAVE_API_VERSION;
options.host = "127.0.0.1";
options.port = 1080;
options.connect_timeout_ms = 5000;
options.reconnect_interval_ms = 3000;
options.log_level = WEAVE_LOG_INFO;
options.log_callback = WriteLog;

if (weave::Configure(options) != WEAVE_OK)
    return;
if (weave::Start() != WEAVE_OK)
    return;

// start application networking here
weave::WriteLog(weave::LogLevel::Info, "application started");

// close application sockets and finish pending I/O first
const auto result = weave::Stop();
// WEAVE_BUSY leaves interception running; stop again after network work has ended
```

Call lifecycle functions outside `DllMain`, TLS callbacks and other loader-lock contexts. Call `Start` before application networking begins. The application still owns `WSAStartup` / `WSACleanup`. Already connected sockets are not migrated to the proxy.

`Configure` copies strings; callback/context must remain valid until `Stop` succeeds. Username and password must either both be absent or both contain 1–255 bytes. Timeouts are milliseconds, from 100 to INT_MAX. Invalid configuration leaves the previous configuration intact. Changing configuration while running returns `WEAVE_BUSY`.

`Start` is idempotent. `Stop` returns `WEAVE_BUSY` while intercepted calls, managed sockets or background operations remain; it never closes application sockets. Successful stop drains logging and leaves installed hooks in pass-through mode. Configuration and startup can then be repeated. Weave and the native module containing the log callback stay loaded until process exit; DLL unloading is unsupported. Lifecycle calls from inside a hook or log callback return `WEAVE_BUSY`. Quiesce network threads before stopping, including blocking completion waits.

The exported ABI is C; the C++ namespace wraps it without passing STL objects across DLL boundaries. Set both `size` and `api_version` in options. API version and package version are separate. Startup failures return `WEAVE_START_FAILED`; `GetLastError` provides the Windows error where available.

## logs

One writer thread invokes the callback outside networking locks. Records retain the producer thread ID and UTC timestamp (Windows FILETIME units), severity, source, file and line. Record pointers only live during the callback. `weave::WriteLog` submits host messages to the same queue; `SetLogLevel` changes the threshold at runtime.

The queue holds up to 1024 pending records. When full, records are dropped and a summary follows; networking does not wait for disk writes. A slow callback can delay `Stop`, so callbacks must finish promptly. Without a callback logging is disabled. Do not call lifecycle methods, block on network threads, or throw across the callback boundary. Logging recursively from the callback is suppressed.

## scope

Supported baseline: Windows x64, IPv4 SOCKS5 TCP CONNECT and UDP ASSOCIATE, including the Winsock overlapped and completion paths ported from droute. Loopback traffic stays direct. This is not a system-wide VPN: it does not intercept every Windows networking API, implement a SOCKS server or provide general IPv6 / ConnectEx / RIO coverage. SOCKS UDP fragmentation is unsupported. Applications using other transports need separate integration work.

## releases

`VERSION` contains `major.minor.patch`, initially `0.0.1`. Build metadata includes the Git revision; the DLL also has a Windows version resource. API-breaking changes must update the API version. For a release, update VERSION, commit, then tag `v<version>`.

The workflow builds on pushes and pull requests to `dev` and on version tags. Tags publish an SDK archive (DLL, import library, header, README and licenses) plus a separate matching PDB. Ordinary builds upload artifacts without publishing a release.

No test projects or test dependencies are kept in this repository. Temporary external checks can be used during development. Follow the naming and formatting style inherited from droute; comments explain non-obvious constraints and start with lowercase letters.

## license

GPL-3.0; see [LICENSE](LICENSE). MinHook is a submodule with its own [license](external/minhook/LICENSE.txt).
