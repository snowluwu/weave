# Weave

**weave** adds SOCKS5 proxy support to Windows applications through Winsock interception. It handles TCP and UDP connections inside the host process and exposes a C API with C++ wrappers.

## Features

- **TCP and UDP:** SOCKS5 CONNECT and UDP ASSOCIATE for supported IPv4 traffic.
- **Asynchronous I/O:** Support for the intercepted Winsock event, completion routine and IOCP paths.
- **UDP reconnection:** Attempts to restore associations after the proxy connection is lost.
- **Explicit lifecycle:** Configure, start and stop interception from the host application.
- **Host-controlled logs:** Asynchronous log delivery through a callback.

Requires Windows x64 and a running SOCKS5 proxy. UDP traffic requires proxy support for UDP ASSOCIATE. See [Networking](docs/networking.md) for routing rules and supported APIs.

## Quick Start

Link `weave_proxy.lib`, include `weave.h` and place `weave_proxy.dll` beside your application. Configure and start weave outside `DllMain` before the application starts networking:

```cpp
#include <weave.h>

weave::Options options{};
options.size = sizeof(options);
options.api_version = WEAVE_API_VERSION;
options.host = "127.0.0.1";
options.port = 1080;
options.connect_timeout_ms = 5000;
options.reconnect_interval_ms = 3000;
options.log_level = WEAVE_LOG_INFO;

if (weave::Configure(options) != WEAVE_OK)
    return;
if (weave::Start() != WEAVE_OK)
    return;

// start application networking
```

The host manages Winsock initialization and its own sockets. Before stopping weave, close managed sockets and finish pending I/O. See [Getting Started](docs/getting-started.md) for a complete example.

## Documentation

- [Getting Started](docs/getting-started.md) — build, link and run weave.
- [API Reference](docs/api.md) — options, results, functions and types.
- [Lifecycle](docs/lifecycle.md) — startup, shutdown and ownership.
- [Networking](docs/networking.md) — routing, supported APIs and limitations.
- [Logging](docs/logging.md) — callbacks, levels and record format.
- [Builds and Releases](docs/builds-and-releases.md) — versioning and release artifacts.

## License

GPL-3.0; see [LICENSE](LICENSE). Weave uses [MinHook](https://github.com/TsudaKageyu/minhook), distributed under its own license in `licenses/MinHook.txt` in the SDK archive.
