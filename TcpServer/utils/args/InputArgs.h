#ifndef KAS_UTILS_ARGS_INPUT_ARGS_H
#define KAS_UTILS_ARGS_INPUT_ARGS_H

#include <cstddef>
#include <iostream>
#include <optional>
#include <string_view>
#include <string>
#include <unordered_map>
#include <vector>

namespace kas::utils::args {

    class InputArgs final {
    public:

        explicit InputArgs(int count, wchar_t* argv[]);

        [[nodiscard]] std::optional<std::wstring_view> getIndex(std::size_t index) const;

        [[nodiscard]] std::optional<std::wstring_view> getKey(std::wstring_view key) const;

        [[nodiscard]] bool hasKey(std::wstring_view key) const;

        [[nodiscard]] std::size_t countIndex() const noexcept;

        [[nodiscard]] std::size_t countKeys() const noexcept;

        void print(std::wostream& out) const;

    private:
        std::vector<std::wstring_view> m_args;
        std::unordered_map<std::wstring_view, std::wstring_view> m_keys;
    };

} // namespace kas::utils::args

#endif