#pragma once

#include "src/hook/hooks.hpp"

namespace weave {

    int ConnectToProxy(SOCKET s, uint64_t deadline);
    int ConnectViaProxy(SOCKET s, const sockaddr_in& target, uint64_t deadline);

}
