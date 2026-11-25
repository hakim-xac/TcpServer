#pragma once

#include <windows.h>
#include <string>

namespace kas::utils::helper
{
    [[nodiscard]] inline
    std::wstring 
    toWide(const char* s, UINT cp = CP_UTF8)
    {
        if (!s)
            return {};

        const int n{ MultiByteToWideChar(cp, 0, s, -1, nullptr, 0) };
        if (n <= 0)
            return {};

        std::wstring w(n - 1, L'\0');
        MultiByteToWideChar(cp, 0, s, -1, w.data(), n);
        if (n <= 0)
            return {};
        return w;
    }
}