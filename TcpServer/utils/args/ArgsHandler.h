#ifndef KAS_UTILS_ARGS_ARGS_HANDLER_H
#define KAS_UTILS_ARGS_ARGS_HANDLER_H

#include "CommandInfo.h"
#include "InputArgs.h"

#include <iosfwd>
#include <string>
#include <vector>

namespace kas::utils::args
{
    class ArgsHandler final {
    public:
        explicit ArgsHandler(int argc, wchar_t* argv[], std::wostream& out);

        explicit ArgsHandler(InputArgs input_args, std::wostream& out);

        CommandInfo& insert(
            std::wstring command_name,
            CommandInfo::function_t func,
            CommandInfo::ShowLevelType show_level_type = CommandInfo::ShowLevelType::User)&;

        bool call() const;

        void print(CommandInfo::PrintAllCommands pac = CommandInfo::PrintAllCommands::NO) const;

        [[nodiscard]] const InputArgs& getInputArgs() const noexcept;

    private:
        InputArgs m_input_args;
        std::wostream* m_out_ptr;
        std::vector<CommandInfo> m_commands;
    };

}

#endif