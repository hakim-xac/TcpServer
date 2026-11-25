#ifndef KAS_UTILS_LOGGER_BASE_INTERFACE_H
#define KAS_UTILS_LOGGER_BASE_INTERFACE_H

#include "LoggerTags.h"

#include <atomic>
#include <chrono>
#include <optional>
#include <source_location>

namespace kas::utils::logger {

    template <typename LoggerType>
    class LoggerBaseInterface {
    public:

        LoggerBaseInterface()
            : m_log_level_at{ 0 }
            , m_log_enabling_at{ true }
        {
        }


        [[nodiscard]] inline bool getEnable() const&
        {
            return m_log_enabling_at.load();
        }

        inline void setEnable(bool flag)&
        {
            m_log_enabling_at.store(flag);
        }

        inline void setEnabled()&
        {
            m_log_enabling_at.store(true);
        }

        inline void setDisabled()&
        {
            m_log_enabling_at.store(false);
        }

        inline void setLogLevel(size_t log_level)&
        {
            m_log_level_at.store(log_level);
        }

        inline size_t getLogLevel() const noexcept
        {
            return m_log_level_at.load();
        }

        template <typename... TypeErrorLevels>
        void setLogLevel()&
        {
            m_log_level_at.store((getErrorLevel<TypeErrorLevels>() + ...));
        }

        template <typename... Args>
        void print(Args&&... args) const
        {
            printWithOutColorStyle(std::forward<Args>(args)...);
        }

        template <typename... Args>
        void printTrace(Args&&... args) const
        {
            printUsualStyleImpl<tags::Trace>(std::forward<Args>(args)...);
        }

        template <typename... Args>
        void printDebug(Args&&... args) const
        {
            printUsualStyleImpl<tags::Debug>(std::forward<Args>(args)...);
        }

        template <typename... Args>
        void printLog(Args&&... args) const
        {
            printUsualStyleImpl<tags::Log>(std::forward<Args>(args)...);
        }

        template <typename... Args>
        void printInfo(Args&&... args) const
        {
            printUsualStyleImpl<tags::Info>(std::forward<Args>(args)...);
        }

        template <typename... Args>
        void printWarn(Args&&... args) const
        {
            printUsualStyleImpl<tags::Warning>(std::forward<Args>(args)...);
        }

        template <typename Arg>
        void printError(Arg&& arg, const std::source_location& location = std::source_location::current()) const
        {
            printVerboselStyleImpl<tags::Error>(location, std::forward<Arg>(arg));
        }

        template <typename Arg>
        void printFatal(Arg&& arg, const std::source_location& location = std::source_location::current()) const
        {
            printVerboselStyleImpl<tags::Fatal>(location, std::forward<Arg>(arg));
        }

        template <typename Arg>
        void printException(Arg&& arg, const std::source_location& location = std::source_location::current()) const
        {
            printVerboselStyleImpl<tags::Exception>(location, std::forward<Arg>(arg));
        }

    private:
        template <typename... Args>
        void printWithOutColorStyle(Args&&... args) const
        {
            static_cast<const LoggerType*>(this)->printWithOutColorStyle(std::forward<Args>(args)...);
        }

        template <kas::concepts::as_simple_color_t TID, typename... Args>
        void printUsualStyleImpl(Args&&... args) const
        {
            if (!m_log_enabling_at) [[unlikely]]
                return;

            if (checkLevel(TID::code)) {
                static_cast<const LoggerType*>(this)->template printUsualStyle<TID>(
                    convertTimePointToString(std::chrono::system_clock::now()).value_or("--"),
                    std::forward<Args>(args)...);
            }
        }

        template <kas::concepts::as_simple_color_t TID, typename Arg>
        void printVerboselStyleImpl(const std::source_location& location, Arg&& arg) const
        {
            if (!m_log_enabling_at) [[unlikely]]
                return;

            if (checkLevel(TID::code)) {
                static_cast<const LoggerType*>(this)->template printVerboselStyle<TID>(
                    location,
                    convertTimePointToString(std::chrono::system_clock::now()).value_or("--"),
                    std::forward<Arg>(arg));
            }
        }

        template <typename TypeErrorLevel>
        [[nodiscard]] size_t getErrorLevel() const noexcept
        {
            return TypeErrorLevel::code;
        }

        [[nodiscard]] constexpr bool checkLevel(std::size_t log_level) const noexcept
        {
            const size_t current_log_level{ m_log_level_at.load() };
            if (current_log_level == 0)
                return true;

            return (current_log_level & log_level) != 0;
        }

        [[nodiscard]] inline std::optional<std::wstring>
            convertTimePointToString(
                const std::chrono::system_clock::time_point& time_point,
                const std::wstring& format = "%Y-%m-%d %H:%M:%S") const
        {
            const std::time_t time{ std::chrono::system_clock::to_time_t(time_point) };
            std::tm timeinfo{ };
#if defined(_WIN32)
            if (localtime_s(&timeinfo, &time) != 0)
                return std::nullopt;
#else
            if (localtime_r(&time, &timeinfo) == nullptr)
                return std::nullopt;
#endif

            char buffer[64]{ };
            const std::size_t write_bytes = std::strftime(buffer, sizeof(buffer), format.c_str(), &timeinfo);
            if (write_bytes == 0) [[unlikely]]
                return std::nullopt;

            return std::wstring{ buffer };
        }

        std::atomic<size_t> m_log_level_at;
        std::atomic_bool m_log_enabling_at;
    };

}

#endif