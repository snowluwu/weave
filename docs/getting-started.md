# Getting Started

## Requirements

- Windows x64 application and a SOCKS5 server reachable from the process.
- UDP ASSOCIATE support on the server when proxying UDP.
- Visual Studio 2022 C++ desktop workload and Windows SDK when building from source.

## Get the SDK

Download `weave-sdk-x64.zip` from the project's releases, or build from source:

```powershell
git clone --recurse-submodules https://github.com/snowluwu/weave.git
cd weave
msbuild weave.sln /p:Configuration=Release /p:Platform=x64
```

For an existing checkout, initialize dependencies with `git submodule update --init --recursive`. The build puts `weave_proxy.dll`, `weave_proxy.lib` and `weave_proxy.pdb` in `dist/x64/Release`.

## Connect the Library

Add `include` to the compiler's include directories, add the directory containing `weave_proxy.lib` to the linker's library directories, and link `weave_proxy.lib`. Deploy the matching `weave_proxy.dll` where Windows can load it, normally beside the executable. Keep the matching PDB for debugging.

## Start and Stop

This minimal C++ example starts the library, leaves a place for application work, and then shuts it down. Real applications must close their sockets and wait for outstanding I/O before calling `Stop`.

```cpp
#include <cstdio>
#include <weave.h>

int main() {
    weave::Options options{};
    options.size = sizeof(options);
    options.api_version = WEAVE_API_VERSION;
    options.host = "127.0.0.1";
    options.port = 1080;
    options.connect_timeout_ms = 5000;
    options.reconnect_interval_ms = 3000;
    options.log_level = WEAVE_LOG_INFO;

    auto result = weave::Configure(options);
    if (result != WEAVE_OK) {
        std::fprintf(stderr, "Configure failed: %d\n", static_cast<int>(result));
        return 1;
    }

    result = weave::Start();
    if (result != WEAVE_OK) {
        std::fprintf(stderr, "Start failed: %d\n", static_cast<int>(result));
        return 1;
    }

    // initialize Winsock and run application networking here
    // close sockets, finish pending I/O and stop network threads here

    result = weave::Stop();
    if (result != WEAVE_OK) {
        std::fprintf(stderr, "Stop failed: %d\n", static_cast<int>(result));
        return 1;
    }
    return 0;
}
```

`WEAVE_BUSY` means weave is still running; end remaining network work and retry. The host is responsible for `WSAStartup` and `WSACleanup`. See [Lifecycle](lifecycle.md) before integrating into an existing process and [Logging](logging.md) to receive diagnostics.
