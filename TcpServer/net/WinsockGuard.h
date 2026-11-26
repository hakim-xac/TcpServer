#pragma once

#include <ws2tcpip.h>
#include <stdexcept>
#include <string>

namespace kas::net
{
    struct WinsockGuard
    {
        WinsockGuard()
        {
            WSADATA wsa{};
            int rc = WSAStartup(MAKEWORD(2, 2), &wsa);
            if (rc != 0) [[unlikely]]
                throw std::runtime_error("WSAStartup failed: " + std::to_string(rc));
        }

        ~WinsockGuard() { WSACleanup(); }
        WinsockGuard(const WinsockGuard&) = delete;
        WinsockGuard(WinsockGuard&&) noexcept = delete;
        WinsockGuard& operator=(const WinsockGuard&) = delete;
        WinsockGuard& operator=(WinsockGuard&&) noexcept = delete;
    };
}