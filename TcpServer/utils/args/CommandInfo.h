#ifndef KAS_UTILS_ARGS_COMMAND_INFO_H
#define KAS_UTILS_ARGS_COMMAND_INFO_H

#include <cstdint>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <functional>

namespace kas::utils::args {

    class CommandInfo final {
    public:
        using function_t = std::function<bool()>;

        enum class ShowLevelType : uint8_t {
            User,
            Developer
        };

        enum class PrintAllCommands : uint8_t {
            YES,
            NO
        };

        struct OptionCommandInfo {
            std::wstring name;
            std::optional<std::wstring> description_opt;
        };

        struct Examples {
            std::wstring description;
            std::vector<std::wstring> options;
        };

        explicit CommandInfo(
            std::wstring command_name,
            function_t func,
            ShowLevelType show_level_type);

        CommandInfo(const CommandInfo&) = delete;
        CommandInfo(CommandInfo&&) noexcept = default;
        CommandInfo& operator=(const CommandInfo&) = delete;
        CommandInfo& operator=(CommandInfo&&) noexcept = default;
        ~CommandInfo() = default;

        [[nodiscard]] bool call() const;

        [[nodiscard]] const std::wstring& getCommandName() const& noexcept;

        [[nodiscard]] ShowLevelType getShowLevelType() const noexcept;

        CommandInfo& setDescription(std::wstring description)&;

        CommandInfo& appendExample(std::wstring description,
            std::vector<std::wstring> options = { })&;

        CommandInfo& appendPrototype(std::wstring prototype)&;

        CommandInfo& appendOption(
            std::wstring name,
            std::optional<std::wstring> description_opt = std::nullopt)&;

        void print(std::wostream& out, std::wstring_view appname) const;

    private:
        std::wstring m_command_name;
        function_t m_func;
        ShowLevelType m_show_level_type;
        std::wstring m_description;
        std::vector<OptionCommandInfo> m_option_list;
        std::vector<Examples> m_examples;
        std::vector<std::wstring> m_prototypes;
    };
}

#endif