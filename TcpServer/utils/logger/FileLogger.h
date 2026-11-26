#pragma once

#include "LoggerSync.h"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <utility>

namespace kas::utils::logger
{
    class FileLogger final {
    public:
        explicit FileLogger(std::filesystem::path filename, std::size_t max_filesize = 0)
            : m_filename{ std::move(filename) }
            , m_file{ openFile(m_filename) }
            , m_logger{ m_file }
            , m_max_filesize{ max_filesize }
            , m_file_mx{ }
        {
        }

        FileLogger(const FileLogger&) = delete;
        FileLogger& operator=(const FileLogger&) = delete;

        template <typename... Args>
        void print(Args&&... args)
        {
            std::lock_guard lk{ m_file_mx };
            checkAndRotateIfNeededLocked();
            m_logger.print(std::forward<Args>(args)...);
        }

        template <typename... Args>
        void printTrace(Args&&... args)
        {
            std::lock_guard lk{ m_file_mx };
            checkAndRotateIfNeededLocked();
            m_logger.printTrace(std::forward<Args>(args)...);
        }

        template <typename... Args>
        void printDebug(Args&&... args)
        {
            std::lock_guard lk{ m_file_mx };
            checkAndRotateIfNeededLocked();
            m_logger.printDebug(std::forward<Args>(args)...);
        }

        template <typename... Args>
        void printLog(Args&&... args)
        {
            std::lock_guard lk{ m_file_mx };
            checkAndRotateIfNeededLocked();
            m_logger.printLog(std::forward<Args>(args)...);
        }

        template <typename... Args>
        void printInfo(Args&&... args)
        {
            std::lock_guard lk{ m_file_mx };
            checkAndRotateIfNeededLocked();
            m_logger.printInfo(std::forward<Args>(args)...);
        }

        template <typename... Args>
        void printWarn(Args&&... args)
        {
            std::lock_guard lk{ m_file_mx };
            checkAndRotateIfNeededLocked();
            m_logger.printWarn(std::forward<Args>(args)...);
        }

        template <typename... Args>
        void printError(Args&&... args)
        {
            std::lock_guard lk{ m_file_mx };
            checkAndRotateIfNeededLocked();
            m_logger.printError(std::forward<Args>(args)...);
        }

        template <typename... Args>
        void printFatal(Args&&... args)
        {
            std::lock_guard lk{ m_file_mx };
            checkAndRotateIfNeededLocked();
            m_logger.printFatal(std::forward<Args>(args)...);
        }

        template <typename... Args>
        void printException(Args&&... args)
        {
            std::lock_guard lk{ m_file_mx };
            checkAndRotateIfNeededLocked();
            m_logger.printException(std::forward<Args>(args)...);
        }

        void setEnable(bool flag) { m_logger.setEnable(flag); }
        void setEnabled() { m_logger.setEnabled(); }
        void setDisabled() { m_logger.setDisabled(); }
        [[nodiscard]] bool getEnable() const { return m_logger.getEnable(); }

        void setLogLevel(std::size_t level) { m_logger.setLogLevel(level); }
        [[nodiscard]] std::size_t getLogLevel() const { return m_logger.getLogLevel(); }

        template <typename... TypeErrorLevels>
        void setLogLevel()
        {
            m_logger.template setLogLevel<TypeErrorLevels...>();
        }

        void flush()
        {
            std::lock_guard lk{ m_file_mx };
            checkAndRotateIfNeededLocked();
            if (m_file.is_open())
                m_file.flush();
        }

        void clearLogFile()
        {
            std::lock_guard lk{ m_file_mx };
            reopen(std::ios::trunc);
        }

        void setFilename(std::filesystem::path filename)
        {
            std::lock_guard lk{ m_file_mx };

            if (m_file.is_open()) {
                m_file.flush();
                m_file.close();
            }

            m_filename = std::move(filename);
            m_file.open(m_filename, std::ios::out | std::ios::app);
            if (!m_file.is_open()) {
                throw std::runtime_error(
                    "FileLogger: cannot open file: " + m_filename.string());
            }
        }

        [[nodiscard]] std::filesystem::path getFilename() const
        {
            std::lock_guard lk{ m_file_mx };
            return m_filename;
        }

        void setMaxFilesize(std::size_t max_filesize) noexcept
        {
            std::lock_guard lk{ m_file_mx };
            m_max_filesize = max_filesize;
        }

        void resetMaxFilesize() noexcept
        {
            std::lock_guard lk{ m_file_mx };
            m_max_filesize = 0;
        }

        [[nodiscard]] std::size_t getMaxFilesize() const noexcept
        {
            std::lock_guard lk{ m_file_mx };
            return m_max_filesize;
        }

    private:

        static std::wofstream openFile(const std::filesystem::path& filename)
        {
            std::error_code ec;
            const auto parent = filename.parent_path();
            if (!parent.empty())
                std::filesystem::create_directories(parent, ec);

            std::wofstream f{ filename, std::ios::out | std::ios::app };
            if (!f.is_open())
                throw std::runtime_error("FileLogger: cannot open file: " + filename.string());
            return f;
        }

        void reopen(std::ios::openmode mode)
        {
            if (m_file.is_open()) {
                m_file.flush();
                m_file.close();
            }
            m_file.open(m_filename, mode | std::ios::out);
            if (!m_file.is_open()) {
                throw std::runtime_error(
                    "FileLogger: cannot reopen file: " + m_filename.string());
            }
        }

        void checkAndRotateIfNeededLocked()
        {
            if (m_max_filesize == 0)
                return;

            if (m_file.is_open())
                m_file.flush();

            std::error_code ec;
            const auto size = std::filesystem::file_size(m_filename, ec);
            if (ec)
                return;

            if (size >= m_max_filesize)
                reopen(std::ios::trunc);
        }

        void checkAndRotateIfNeeded()
        {
            std::lock_guard lk{ m_file_mx };
            checkAndRotateIfNeededLocked();
        }

        std::filesystem::path m_filename;
        std::wofstream m_file;
        LoggerSync m_logger;
        std::size_t m_max_filesize;

        mutable std::mutex m_file_mx;
    };
}