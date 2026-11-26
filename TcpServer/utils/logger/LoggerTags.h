#ifndef KAS_UTILS_LOGGER_LOGGER_TAGS_H
#define KAS_UTILS_LOGGER_LOGGER_TAGS_H

#include <string_view>

namespace kas::utils::logger::tags
{
    struct Default;

    struct Trace {
        constexpr static size_t code{ 1 };
        constexpr static std::wstring_view header{ L"Trace" };
    };

    struct Debug {
        constexpr static size_t code{ 2 };
        constexpr static std::wstring_view header{ L"Debug" };
    };
    struct Log {
        constexpr static size_t code{ 4 };
        constexpr static std::wstring_view header{ L"Log" };
    };
    struct Info {
        constexpr static size_t code{ 8 };
        constexpr static std::wstring_view header{ L"Info" };
    };
    struct Warning {
        constexpr static size_t code{ 16 };
        constexpr static std::wstring_view header{ L"Warning" };
    };
    struct Error {
        constexpr static size_t code{ 32 };
        constexpr static std::wstring_view header{ L"Error" };
    };
    struct Fatal {
        constexpr static size_t code{ 64 };
        constexpr static std::wstring_view header{ L"Fatal" };
    };
    struct Exception {
        constexpr static size_t code{ 128 };
        constexpr static std::wstring_view header{ L"Exception" };
    };

}

namespace kas::concepts {
    template <typename T>
    concept as_logger_t = std::same_as<T, kas::utils::logger::tags::Default>
        || requires {
        T::code;
        T::header;
    };
}

#endif