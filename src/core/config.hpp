#pragma once
#include "pch.h"
#include "src/core/logger.hpp"
#include "weave.h"

namespace weave {
    struct Config {
        std::string host;
        uint16_t port = 0;
        std::string user;
        std::string password;
        uint32_t connectTimeout = 5000;
        uint32_t reconnectInterval = 3000;
        Severity logLevel = Severity::Info;
        WeaveLogCallback logCallback = nullptr;
        void* logContext = nullptr;
    };

    extern Config g_cfg;
    extern sockaddr_in g_proxyAddr;
    void ResetProxyAddress();
    bool GetProxyAddress(sockaddr_in& address);
    bool IsResolvingProxy();
}
