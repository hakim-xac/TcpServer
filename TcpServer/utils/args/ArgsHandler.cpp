#include "ArgsHandler.h"
#include "../helper.h"

#include <algorithm>
#include <ranges>
#include <iostream>
#include <iomanip>

namespace kas::utils::args {
    namespace {

        //------------------

        void printError(std::wostream& out, std::wstring_view msg)
        {
            std::wcerr << L"Error: " << msg << L"\n";
        }

        //------------------
    }

    //------------------

    ArgsHandler::ArgsHandler(int argc, wchar_t* argv[], std::wostream& out)
        : m_input_args{ argc, argv }
        , m_out_ptr{ std::addressof(out) }
        , m_commands{ }
    {
    }

    //------------------

    ArgsHandler::ArgsHandler(InputArgs input_args, std::wostream& out)
        : m_input_args{ std::move(input_args) }
        , m_out_ptr{ std::addressof(out) }
        , m_commands{ }
    {
    }

    //------------------

    bool ArgsHandler::call() const
    {
        if (m_input_args.countIndex() == 1) [[unlikely]] {
            print(CommandInfo::PrintAllCommands::NO);
            return false;
        }

        if (m_commands.empty()) [[unlikely]] {
            print(CommandInfo::PrintAllCommands::NO);
            return false;
        }

        const std::optional<std::wstring_view> input_command_name_opt{ m_input_args.getIndex(1) };
        if (!input_command_name_opt) [[unlikely]] {
            print(CommandInfo::PrintAllCommands::NO);
            return false;
        }

        const std::wstring_view command_name{ input_command_name_opt.value() };

        const auto command_fn{
            std::find_if(
                std::cbegin(m_commands), std::cend(m_commands),
                [command_name](const CommandInfo& cinfo) {
                    return command_name == cinfo.getCommandName();
                })
        };

        if (command_fn == std::cend(m_commands)) [[unlikely]] {
            printError(*m_out_ptr, L"The command was not found!");
            print(CommandInfo::PrintAllCommands::NO);
            return false;
        }

        try {
            return command_fn->call();
        }
        catch (const std::exception& ex) {
            printError(*m_out_ptr, helper::toWide( ex.what()));
            return false;
        }
    }

    //------------------

    void ArgsHandler::print(CommandInfo::PrintAllCommands pac) const
    {
        const std::wstring_view appname{ m_input_args.getIndex(0).value_or(L"{appname}") };

        auto& out{ *m_out_ptr };
        out << appname << L" " << L"< " << L"command" << L" > ";
        out << appname << L" [ " << L"options" << L" ]\n";

        if (m_commands.empty()) {
            out << L"No commands available\n";
            return;
        }

        out << L"Commands:\n";

        size_t command_id{ };

        constexpr std::size_t tab_count_spaces{ 2 };

        constexpr std::size_t tab_offset{ 1 };

        std::ranges::for_each(
            m_commands, [&](const CommandInfo& command) {
                if (!(command.getShowLevelType() == CommandInfo::ShowLevelType::User || pac == CommandInfo::PrintAllCommands::YES))
                    return;

                out << std::setw(tab_count_spaces) << L" " << (++command_id) << L")";

                command.print(out, appname);
            });
    }

    //------------------

    CommandInfo& ArgsHandler::insert(
        std::wstring command_name,
        CommandInfo::function_t func,
        CommandInfo::ShowLevelType show_level_type)&
    {
        return m_commands.emplace_back(std::move(command_name), std::move(func), show_level_type);
    }

    //------------------
    
    const InputArgs& ArgsHandler::getInputArgs() const noexcept
    {
        return m_input_args;
    }

    //------------------
}