#ifndef KAS_UTILS_LOGGER_LOGGER_SYNC_H
#define KAS_UTILS_LOGGER_LOGGER_SYNC_H

#include "LoggerBaseInterface.h"

#include <iostream>
#include <mutex>

namespace kas::utils::logger {

    class LoggerSync : public LoggerBaseInterface<LoggerSync> {
    public:

        explicit LoggerSync(std::wostream& out = std::wcout)
            : m_out_ptr{ std::addressof(out) }
            , m_mx{ }
        {
        }

        const LoggerSync& flush() const&;

        LoggerSync&& flush()&&;

    protected:
        std::wostream* m_out_ptr;
        mutable std::mutex m_mx;
        
        template <typename Arg, typename... Args>
        void printWithOutColorStyleImpl(Arg&& arg, Args&&... args) const
        {
            auto& out{ *m_out_ptr };
            std::lock_guard lk{ m_mx };
            out << std::forward<Arg>(arg);
            if constexpr (sizeof...(args) > 0)
            {
                ((out << L" " << std::forward<Args>(args)), ...);
            }
        }

        template <kas::concepts::as_simple_color_t TID, typename Arg, typename... Args>
        void printUsualStyleImpl(const std::wstring& time,Arg&& arg, Args&&... args) const
        {
            auto& out{ *m_out_ptr };
            std::lock_guard lk{ m_mx };
            out << L"[" << time << L"]" << L" " << L"(" << TID::header << L")" << L" " << std::forward<Arg>(arg);
            if constexpr (sizeof...(args) > 0)
            {
                ((out << ' ' << std::forward<Args>(args)), ...);
            }
            out << L"\n";
        }

        template <kas::concepts::as_simple_color_t TID, typename Arg, typename... Args>
        void printVerboselStyleImpl(const std::source_location& location, const std::wstring& time, Arg&& arg, Args&&... args) const
        {
            auto& out{ *m_out_ptr };
            std::lock_guard lk{ m_mx };
            out << L"[" << time << L"]" << L" " << L"(" << TID::header << L")" << L" " << std::forward<Arg>(arg);
            if constexpr (sizeof...(args) > 0)
            {
                ((out << L" " << std::forward<Args>(args)), ...);
            }
            out << L" " << L"(" << L" " << location.file_name() << L" " << L":" << L" " << location.line() << L" " << L")" << L" ";
            out << L"->" << L" " << location.function_name() << L"\n";
        }

    private:
        friend LoggerBaseInterface<LoggerSync>;

        template <typename... Args>
        void printWithOutColorStyle(Args&&... args) const
        {
            printWithOutColorStyleImpl(std::forward<Args>(args)...);
        }

        template <kas::concepts::as_simple_color_t TID, typename... Args>
        void printUsualStyle(const std::wstring& time, Args&&... args) const
        {
            printUsualStyleImpl<TID>(time, std::forward<Args>(args)...);
        }
        template <kas::concepts::as_simple_color_t TID, typename... Args>
        void printVerboselStyle(const std::source_location& location,
            const std::wstring& time, Args&&... args) const
        {
            printVerboselStyleImpl<TID>(location, time, std::forward<Args>(args)...);
        }
    };

}

namespace kas::utils::logger {

    inline const LoggerSync&
        LoggerSync::flush() const&
    {
        std::lock_guard lk{ m_mx };
        m_out_ptr->flush();
        return *this;
    }

    inline LoggerSync&&
        LoggerSync::flush()&&
    {
        flush();
        return std::move(*this);
    }
}

#endif