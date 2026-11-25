#include "CommandInfo.h"

#include <iostream>
#include <iomanip>

namespace kas::utils::args
{
    //------------------

    CommandInfo::CommandInfo(
        std::wstring command_name,
        CommandInfo::function_t func,
        ShowLevelType show_level_type)
        : m_command_name{ std::move(command_name) }
        , m_func{ std::move(func) }
        , m_show_level_type{ show_level_type }
        , m_description{ }
    {
    }

    //------------------

    [[nodiscard]]
    CommandInfo::ShowLevelType
        CommandInfo::getShowLevelType() const noexcept
    {
        return m_show_level_type;
    }

    //------------------

    bool CommandInfo::call() const
    {
        return std::invoke(m_func);
    }

    //------------------

    [[nodiscard]]
    const std::wstring&
        CommandInfo::getCommandName() const& noexcept
    {
        return m_command_name;
    }

    //------------------

    CommandInfo& CommandInfo::setDescription(std::wstring description_text)&
    {
        std::swap(m_description, description_text);
        return *this;
    }

    //------------------

    CommandInfo& CommandInfo::appendExample(std::wstring description, std::vector<std::wstring> options)&
    {
        m_examples.emplace_back(std::move(description), std::move(options));
        return *this;
    }

    //------------------

    CommandInfo& CommandInfo::appendPrototype(std::wstring prototype)&
    {
        m_prototypes.emplace_back(std::move(prototype));
        return *this;
    }

    //------------------

    CommandInfo& CommandInfo::appendOption(std::wstring name, std::optional<std::wstring> description_opt)&
    {
        m_option_list.emplace_back(std::move(name), std::move(description_opt));
        return *this;
    }

    //------------------

    void CommandInfo::print(std::wostream& out, std::wstring_view appname) const
    {
        out << m_command_name << L" - " << (!m_description.empty() ? m_description : L"-------") << L"\n";

        constexpr std::size_t tab_count_spaces{ 2 };
        if (!m_prototypes.empty()) {
            constexpr std::size_t tab_offset{ 1 };
        
            out << std::setw(tab_count_spaces) << L" " << L"Prototype:\n";

            for (const auto& elem : m_prototypes) {
                out << std::setw(tab_count_spaces << tab_offset) << L" " << appname << m_command_name << elem << L"\n";
            }
        }

        if (!m_option_list.empty()) {
            constexpr std::size_t tab_offset{ 1 };

            out << std::setw(tab_count_spaces) << L" " << L"Options:\n";

            for (const auto& option : m_option_list) {
                out << std::setw(tab_count_spaces << tab_offset) << L" " << option.name;
                if (option.description_opt)
                    out << L" - " << option.description_opt.value();

               out << L"\n";
            }
        }

        if (!m_examples.empty()) {
            constexpr std::size_t tab_offset{ 1 };
            out << std::setw(tab_count_spaces) << L" "  << L"Example:\n";

            for (const auto& elem : m_examples) {
                out << std::setw(tab_count_spaces << tab_offset) << L" " << appname << m_command_name;

                if (!elem.options.empty()) {

                    for (const auto& option : elem.options) {
                        out << L" " << option;
                    }
                }
                out << L"\n";

                if (!elem.description.empty())
                {
                    constexpr std::size_t tab_offset{ 2 };
                    out << std::setw(tab_count_spaces << tab_offset) << L" " << elem.description << L"\n";
                }
            }
           out << L"\n";
        }
    }

    //------------------
}