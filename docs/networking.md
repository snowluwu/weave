# Networking

## Scope

Weave intercepts selected Winsock and Windows completion functions inside the current process. It is not a system-wide proxy or VPN. It implements SOCKS5 TCP CONNECT and UDP ASSOCIATE for IPv4 destinations; it does not provide a SOCKS server.

The application resolves its destinations and passes socket addresses to Winsock. Weave does not offer remote DNS for application hostnames. A configured proxy hostname is resolved to IPv4 locally when needed and the successful result is cached until the next configuration.

## Routing

IPv4 loopback (`127.0.0.0/8`), private (`10.0.0.0/8`, `172.16.0.0/12`, `192.168.0.0/16`) and link-local (`169.254.0.0/16`) destinations are passed directly. Traffic matching the configured numeric proxy address and port is also passed directly. A proxy configured by hostname is resolved for proxy connections, but the direct-route comparison does not use that resolved address. UDP multicast (`224.0.0.0/4`) is passed directly. IPv6 and other address families follow their original APIs rather than SOCKS5.

These rules are built in; the API does not provide a route list or multiple proxies. Passing traffic directly means it does not use the SOCKS server.

## TCP and UDP

For supported TCP connections, weave connects to the proxy, performs SOCKS5 negotiation and requests CONNECT. It supports the existing blocking and nonblocking connection paths. Proxy failures are reported through the intercepted socket operation or its completion path.

For supported UDP traffic, weave establishes a TCP control connection and requests UDP ASSOCIATE. Datagrams use the returned relay and carry SOCKS5 headers. When the control connection is lost, the worker periodically attempts to restore the association while the socket is still managed. Recovery of an application-level session depends on the application and proxy; weave cannot guarantee that previously sent datagrams are delivered or that an application automatically resumes.

SOCKS5 UDP fragmentation is unsupported. Packets with a nonzero fragmentation field are not decoded. The SOCKS server must support UDP ASSOCIATE and its relay must be reachable from the application.

## Intercepted APIs

The current hooks cover `connect`, `bind`, `closesocket`, `send`, `recv`, `sendto`, `recvfrom`, `WSASend`, `WSARecv`, `WSASendTo`, `WSARecvFrom`, `select`, `WSAPoll`, `getsockopt`, `getpeername`, `WSAEventSelect`, `WSAAsyncSelect`, `WSAEnumNetworkEvents`, `WSAGetOverlappedResult`, `ioctlsocket`, `CreateIoCompletionPort`, `GetQueuedCompletionStatus`, `GetQueuedCompletionStatusEx`, `GetOverlappedResult`, `GetOverlappedResultEx` and `CancelIoEx`. These hooks also support the corresponding event, completion routine and IOCP handling implemented for managed operations.

This list does not imply that every mode or combination of these APIs is supported. `ConnectEx`, Registered I/O (RIO), general IPv6 proxying and other networking stacks are outside the current scope. Check the application's actual networking path before relying on interception.

## Authentication

Weave supports SOCKS5 without credentials and username/password authentication. Supply both username and password, or neither. The API does not encrypt credentials; transport security depends on the connection to the proxy.
