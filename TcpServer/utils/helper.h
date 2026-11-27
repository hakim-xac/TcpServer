#pragma once

// Отключаем автоподключение winsock.h из windows.h, чтобы не конфликтовало с winsock2.h.
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include <windows.h>
#include <string>
#include <filesystem>

namespace kas::utils::helper
{
    // Преобразует узкую строку (по умолчанию UTF-8) в широкую (UTF-16).
    // Возвращает пустую строку при ошибке или nullptr.
    [[nodiscard]] inline std::wstring 
    toWide(const char* s, UINT cp = CP_UTF8)
    {
        if (!s)
            return {};

        const int n{ MultiByteToWideChar(cp, 0, s, -1, nullptr, 0) };
        if (n <= 0)
            return {};

        std::wstring w(n, L'\0');
        const int r{ MultiByteToWideChar(cp, 0, s, -1, w.data(), n) };

        if (r <= 0)
            return {};

        w.resize(r - 1);
        return w;
    }

    // Возвращает каталог, в котором лежит текущий exe.
    // При ошибке - текущий рабочий каталог.
    [[nodiscard]] inline std::filesystem::path
    getExeDirectory()
    {
        wchar_t buf[MAX_PATH]{};
        const DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
        if (n == 0 || n == MAX_PATH)
            return std::filesystem::current_path();

        return std::filesystem::path{ buf }.parent_path();
    }
}