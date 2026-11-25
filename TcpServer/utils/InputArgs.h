#ifndef KAS_UTILS_ARGS_H
#define KAS_UTILS_ARGS_H

#include <cstddef>
#include <iostream>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace kas::utils::args {

    class InputArgs final {
    public:

        explicit InputArgs(int count, char* argv[]);

        [[nodiscard]] std::optional<std::string_view> getIndex(std::size_t index) const;

        [[nodiscard]] std::optional<std::string_view> getKey(std::string_view key) const;

        [[nodiscard]] bool hasKey(std::string_view key) const;

        [[nodiscard]] std::size_t countIndex() const noexcept;

        [[nodiscard]] std::size_t countKeys() const noexcept;

        void print(std::ostream& out) const;

    private:
        std::vector<std::string_view> m_args;
        std::unordered_map<std::string_view, std::string_view> m_keys;
    };

} // namespace kas::utils::args

#endif