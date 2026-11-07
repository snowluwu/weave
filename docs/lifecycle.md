# Lifecycle

## Configure

`Configure` validates and copies options without installing hooks. Configure while the engine is stopped. A new configuration replaces the old one only after validation succeeds; `WEAVE_BUSY` means it is still running. The API exposes one engine and one proxy configuration per process.

## Start

Call `Start` after a successful configuration and before the application begins networking. Loading the DLL alone does not start weave. The host initializes Winsock with `WSAStartup` when needed; weave does not own the host's Winsock lifetime. Connections already established before startup are not moved through the proxy.

Do not call lifecycle functions from `DllMain`, TLS callbacks or another loader-lock context. Initialize from normal application code once the loader has finished. When integrating through an executable import, network calls made by other initializers before that point are outside this guarantee.

`Start` creates background workers and installs hooks on the first successful start. Calling it again while running returns `WEAVE_OK`. Startup does not prove that the proxy is reachable; connections are attempted when traffic needs them.

## Stop and Restart

Stop creating network operations, close managed sockets and let their pending operations and callbacks finish before calling `Stop`. If an intercepted call, managed socket or background I/O is still active, `Stop` returns `WEAVE_BUSY` and leaves the engine running. Blocking completion waits can also keep it busy. End that work and retry; do not treat `WEAVE_BUSY` as successful shutdown.

A successful stop waits for the worker and log callback thread to finish, drains pending log records and stops interception. Hooks remain installed in pass-through mode. The host may then configure and start weave again; sockets created before a restart are not retroactively converted.

Lifecycle calls are serialized. Calls from an intercepted call or the log callback return `WEAVE_BUSY`. Do not wait for a network thread from the log callback: that can prevent clean shutdown.

## Ownership

Weave never closes application-owned sockets to make `Stop` succeed. It owns internal proxy connections, workers and log queue. Configuration strings are copied; callback code, callback context and any managed delegate remain the host's responsibility until shutdown succeeds. Record pointers are valid only during the callback.

Weave pins its DLL once startup begins and does not support unloading it during the process lifetime, even after `Stop`. The host must also keep callback code loaded until `Stop` succeeds. A blocked callback can delay `Stop` indefinitely.
