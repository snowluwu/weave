#include "pch.h"
#include "src/core/config.hpp"

namespace weave {
    Config g_cfg;
    sockaddr_in g_proxyAddr;
    static INIT_ONCE g_resolveOnce = INIT_ONCE_STATIC_INIT;
    static sockaddr_in g_resolvedProxy = {};
    static thread_local bool g_resolvingProxy = false;

    bool IsResolvingProxy() {
        return g_resolvingProxy;
    }

    static BOOL CALLBACK ResolveProxy(PINIT_ONCE, void*, void**) {
        g_resolvingProxy = true;

        struct ResolveScope {
            ~ResolveScope() {
                g_resolvingProxy = false;
            }
        } scope;

        addrinfo hints = {};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        addrinfo* result = nullptr;
        int error = getaddrinfo(g_cfg.host.c_str(), nullptr, &hints, &result);
        if (error || !result) {
            LOG_ERROR("failed to resolve proxy host '%s': %d", g_cfg.host.c_str(), error);
            return FALSE;
        }
        g_resolvedProxy = g_proxyAddr;
        g_resolvedProxy.sin_addr = reinterpret_cast<sockaddr_in*>(result->ai_addr)->sin_addr;
        freeaddrinfo(result);
        return TRUE;
    }

    bool GetProxyAddress(sockaddr_in& address) {
        if (g_proxyAddr.sin_addr.s_addr != INADDR_NONE) {
            address = g_proxyAddr;
            return true;
        }
        if (!InitOnceExecuteOnce(&g_resolveOnce, ResolveProxy, nullptr, nullptr)) {
            WSASetLastError(WSAHOST_NOT_FOUND);
            return false;
        }
        address = g_resolvedProxy;
        return true;
    }


    void ResetProxyAddress() {
        g_proxyAddr = {};
        g_proxyAddr.sin_family = AF_INET;
        g_proxyAddr.sin_port = htons(g_cfg.port);
        if (inet_pton(AF_INET, g_cfg.host.c_str(), &g_proxyAddr.sin_addr) != 1)
            g_proxyAddr.sin_addr.s_addr = INADDR_NONE;
        g_resolveOnce = INIT_ONCE_STATIC_INIT;
        g_resolvedProxy = {};
    }
}
