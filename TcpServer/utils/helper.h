#pragma once

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include <windows.h>
#include <string>
#include <filesystem>

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

    [[nodiscard]] inline
    std::filesystem::path
    getExeDirectory()
    {
        wchar_t buf[MAX_PATH]{};
        const DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
        if (n == 0 || n == MAX_PATH)
            return std::filesystem::current_path();

        return std::filesystem::path{ buf }.parent_path();
    }
}