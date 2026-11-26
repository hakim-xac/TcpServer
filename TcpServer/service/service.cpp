#include "service.h"
#include "../utils/utils.h"
#include "../net/net.h"
#include "../GlobalVariables.h"

#include <windows.h>
#include <shlobj.h>

#include <string>
#include <memory>
#include <atomic>

namespace kas::service
{
    namespace
    {
        SERVICE_STATUS G_SERVICE_STATUS{};
        SERVICE_STATUS_HANDLE G_SERVICE_STATUS_HANDLE{};
        HANDLE G_STOP_EVENT_SERVICE{ nullptr };

        std::atomic_bool G_STOP_EVENT{};

        std::atomic<kas::net::TcpServer*> G_SERVER{ nullptr };

        constexpr wchar_t G_SERVICE_NAME[] = L"KasService";
        constexpr wchar_t G_SERVICE_DISPLAY_NAME[] = L"Kas Service";

        //=========================================

        void WINAPI handler(DWORD ctrl)
        {
            switch (ctrl) {
            case SERVICE_CONTROL_STOP:
            case SERVICE_CONTROL_SHUTDOWN:
                G_SERVICE_STATUS.dwCurrentState = SERVICE_STOP_PENDING;
                SetServiceStatus(G_SERVICE_STATUS_HANDLE, &G_SERVICE_STATUS);

                G_STOP_EVENT.store(true, std::memory_order_release);
                if (auto* srv = G_SERVER.load(std::memory_order_acquire))
                    srv->stop();

                if (G_STOP_EVENT_SERVICE)
                    SetEvent(G_STOP_EVENT_SERVICE);
                return;
            case SERVICE_CONTROL_INTERROGATE:
                    break;
            default:
                break;
            }
            SetServiceStatus(G_SERVICE_STATUS_HANDLE, &G_SERVICE_STATUS);
        }

        //------------------

        void WINAPI serviceMain(DWORD, LPWSTR*)
        {
            G_SERVICE_STATUS_HANDLE = RegisterServiceCtrlHandlerW(L"KasService", handler);
            if (!G_SERVICE_STATUS_HANDLE)
                return;

            G_SERVICE_STATUS.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
            G_SERVICE_STATUS.dwCurrentState = SERVICE_START_PENDING;
            G_SERVICE_STATUS.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
            G_SERVICE_STATUS.dwCheckPoint = 1;
            G_SERVICE_STATUS.dwWaitHint = 30000;
            SetServiceStatus(G_SERVICE_STATUS_HANDLE, &G_SERVICE_STATUS);

            G_STOP_EVENT_SERVICE = CreateEventW(nullptr, TRUE, FALSE, nullptr);
            if (!G_STOP_EVENT_SERVICE)
            {
                G_SERVICE_STATUS.dwCurrentState = SERVICE_STOPPED;
                G_SERVICE_STATUS.dwWin32ExitCode = GetLastError();
                SetServiceStatus(G_SERVICE_STATUS_HANDLE, &G_SERVICE_STATUS);
                return;
            }

            G_SERVICE_STATUS.dwCurrentState = SERVICE_RUNNING;
            G_SERVICE_STATUS.dwCheckPoint = 0;
            G_SERVICE_STATUS.dwWaitHint = 0;
            SetServiceStatus(G_SERVICE_STATUS_HANDLE, &G_SERVICE_STATUS);

            runServiceBody();

            G_SERVICE_STATUS.dwCurrentState = SERVICE_STOPPED;
            SetServiceStatus(G_SERVICE_STATUS_HANDLE, &G_SERVICE_STATUS);

            CloseHandle(G_STOP_EVENT_SERVICE);
            G_STOP_EVENT_SERVICE = nullptr;
        }

        //------------------
        
        [[nodiscard]]
        std::optional<std::wstring >
        currentExePath()
        {
            std::wstring buf(MAX_PATH, L'\0');
            for (;;)
            {
                const DWORD n{ GetModuleFileNameW(nullptr, buf.data(), static_cast<DWORD>(buf.size())) };
                if (n == 0)
                    return {};
                if (n < buf.size())
                {
                    buf.resize(n);
                    return buf;
                }
                buf.resize(buf.size() * 2);
            }
        }

        //------------------
        
        [[nodiscard]]
        std::optional<std::wstring>
        makeServiceLogPath()
        {
            PWSTR p = nullptr;
            HRESULT hr = SHGetKnownFolderPath(FOLDERID_ProgramData, KF_FLAG_CREATE, nullptr, &p);
            if (FAILED(hr) || !p)
                return std::nullopt;

            std::wstring dir = std::wstring(p) + L"\\Kas\\TcpServer\\logs";
            CoTaskMemFree(p);

            int rc = SHCreateDirectoryExW(nullptr, dir.c_str(), nullptr);
            if (rc != ERROR_SUCCESS && rc != ERROR_ALREADY_EXISTS && rc != ERROR_FILE_EXISTS)
            {
                return std::nullopt;
            }

            return dir + L"\\TcpServerAsService.log";
        }

        //------------------
    }

    //------------------
        
    bool tryRunAsService()
    {
        SERVICE_TABLE_ENTRYW table[] = {
            { const_cast<LPWSTR>(G_SERVICE_NAME), serviceMain },
            { nullptr, nullptr }
        };

        if (StartServiceCtrlDispatcherW(table))
            return true;

        if (GetLastError() == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT)
            return false;

        return false;
    }

    //------------------

    bool installService()
    {
        kas::utils::logger::LoggerSync log{};

        const auto path_opt{ currentExePath() };
        if (!path_opt) [[unlikely]]
        {
            log.print(L"Unable current exe path!");
            return false;
        }

        SC_HANDLE scm{ nullptr };
        utils::FuncGuard _scm_guard{
            [&scm] {
                scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
            },
            [&scm] {
                if (scm)
                CloseServiceHandle(scm);
            }
        };

        if (!scm) [[unlikely]]
        {
            DWORD err = GetLastError();
            if (err == ERROR_ACCESS_DENIED)
                log.print(L"Access denied: run as Administrator");
            else
                log.print(L"OpenSCManagerW failed, error=" + std::to_wstring(err));
            return false;
        }

        SC_HANDLE svc{ nullptr };
        utils::FuncGuard _svc_guard{
            [&svc , &scm, &path_opt] {
                svc = CreateServiceW(
                    scm,
                    G_SERVICE_NAME,
                    G_SERVICE_DISPLAY_NAME,
                    SERVICE_ALL_ACCESS,
                    SERVICE_WIN32_OWN_PROCESS,
                    SERVICE_AUTO_START,
                    SERVICE_ERROR_NORMAL,
                    path_opt->c_str(),
                    nullptr, nullptr, nullptr, nullptr, nullptr);
            },
            [&svc] {
                if (svc)
                CloseServiceHandle(svc);
            }
        };

        bool ok = (svc != nullptr);
        DWORD err = ok ? ERROR_SUCCESS : GetLastError();

        if (!ok && err == ERROR_SERVICE_EXISTS)
        {
            log.print(L"The service has been successfully installed!");
            return true;
        }
        log.print(L"The service has been successfully installed!");

        return ok;
    }

    //------------------

    bool unInstallService()
    {
        kas::utils::logger::LoggerSync log{};

        SC_HANDLE scm{ nullptr };
        utils::FuncGuard _scm_guard { 
            [&scm] {
                scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
            },
            [&scm] {
                if(scm)
                CloseServiceHandle(scm);
            }
        };

        if (!scm) [[unlikely]]
        {
            DWORD err = GetLastError();
            if (err == ERROR_ACCESS_DENIED)
                log.print(L"OpenSCManagerW: access denied (run as Administrator)");
            else
                log.print(L"OpenSCManagerW failed, error=" + std::to_wstring(err));
            return false;
        }

        SC_HANDLE svc{ nullptr };
        utils::FuncGuard _svc_guarg {
            [&svc, &scm] {
                svc = OpenServiceW(scm, G_SERVICE_NAME, SERVICE_STOP | DELETE | SERVICE_QUERY_STATUS);
            },
            [&svc] {
                if (svc)
                CloseServiceHandle(svc);
            }
        };

        if (!svc) [[unlikely]]
        {
            DWORD err = GetLastError();
            if (err == ERROR_SERVICE_DOES_NOT_EXIST)
            {
                log.print(L"Service already absent: " + std::wstring{ G_SERVICE_NAME });
                return true;
            }
            log.print(L"OpenServiceW failed, error=" + std::to_wstring(err));
            return false;
        }

        SERVICE_STATUS status{};
        if (ControlService(svc, SERVICE_CONTROL_STOP, &status))
        {
            // Ожидаем пока служба остановится. максимум ~10 сек. (100 итераций и сон по 100мс.)
            for (int i = 0; i < 100; ++i)
            {
                if (!QueryServiceStatus(svc, &status))
                {
                    log.print(L"QueryServiceStatus failed, error=" +
                        std::to_wstring(GetLastError()));
                    break;
                }
                if (status.dwCurrentState == SERVICE_STOPPED)
                    break;
                Sleep(100);
            }
        }
        else
        {
            DWORD e = GetLastError();
            if (e != ERROR_SERVICE_NOT_ACTIVE)
                log.print(L"ControlService(STOP) failed, error=" + std::to_wstring(e));
        }

        bool ok = DeleteService(svc) != FALSE;

        if (!ok)
        {
            DWORD err = ok ? ERROR_SUCCESS : GetLastError();
            if (err == ERROR_SERVICE_MARKED_FOR_DELETE)
                return true;
            log.print(L"DeleteService failed, error=" + std::to_wstring(err));
        }
        else
        {
            log.print("The service has been successfully removed!");
        }
        return ok;
    }

    //------------------
    
    bool startService()
    {
        kas::utils::logger::LoggerSync log{};

        SC_HANDLE scm{ nullptr };
        utils::FuncGuard _scm_guard{
            [&scm] {
                scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
            },
            [&scm] {
                if (scm)
                CloseServiceHandle(scm);
            }
        };

        if (!scm) [[unlikely]]
        {
            log.printWarn(std::wstring_view{ L"OpenSCManagerW: access denied (run as Administrator)" });
            return false;
        }

        SC_HANDLE svc{ nullptr };
        utils::FuncGuard _svc_guarg{
            [&svc, &scm] {
                svc = OpenServiceW(scm, G_SERVICE_NAME, SERVICE_START | SERVICE_QUERY_STATUS);
            },
            [&svc] {
                if (svc)
                CloseServiceHandle(svc);
            }
        };

        if (!svc) [[unlikely]]
        {
            DWORD err{ GetLastError() };
            if (err == ERROR_SERVICE_DOES_NOT_EXIST)
            {
                log.printWarn(L"Service already absent: " + std::wstring{ G_SERVICE_NAME });
                return true;
            }
            log.printWarn(L"OpenServiceW failed, error=" + std::to_wstring(err));
            return false;
        }

        BOOL ok = StartServiceW(svc, 0, nullptr);
        DWORD err = GetLastError();

        if (!ok && err != ERROR_SERVICE_ALREADY_RUNNING)
        {
            return false;
        }

        SERVICE_STATUS ss{};
        // ждем максимум 5 сек (50 тактов по 100мс) для получения статуса SERVICE_RUNNING
        for (int i = 0; i < 50; ++i)
        {
            if (!QueryServiceStatus(svc, &ss))
                break;
            if (ss.dwCurrentState == SERVICE_RUNNING)
                break;
            if (ss.dwCurrentState == SERVICE_STOPPED)
                break;
            Sleep(100);
        }

        log.print(L"Server started successfully!");

        return ss.dwCurrentState == SERVICE_RUNNING;
    }

    //------------------
    
    bool stopService()
    {
        kas::utils::logger::LoggerSync log{};

        SC_HANDLE scm{ nullptr };
        utils::FuncGuard _scm_guard{
            [&scm] { scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT); },
            [&scm] { if (scm) CloseServiceHandle(scm); }
        };

        if (!scm) [[unlikely]]
        {
            log.printWarn(std::wstring(L"OpenSCManagerW: access denied (run as Administrator)"));
            return false;
        }

        SC_HANDLE svc{ nullptr };
        utils::FuncGuard _svc_guard{
            [&svc, &scm] { svc = OpenServiceW(scm, G_SERVICE_NAME, SERVICE_STOP | SERVICE_QUERY_STATUS); },
            [&svc] { if (svc) CloseServiceHandle(svc); }
        };

        if (!svc) [[unlikely]]
        {
            const DWORD err = GetLastError();
            if (err == ERROR_SERVICE_DOES_NOT_EXIST)
            {
                log.printWarn(L"Service not installed: " + std::wstring{ G_SERVICE_NAME });
                return false;
            }
            log.printWarn(L"OpenServiceW failed, error=" + std::to_wstring(err));
            return false;
        }

        SERVICE_STATUS ss{};
        if (!QueryServiceStatus(svc, &ss))
        {
            log.printWarn(L"QueryServiceStatus failed, error=" + std::to_wstring(GetLastError()));
            return false;
        }

        if (ss.dwCurrentState == SERVICE_STOPPED)
        {
            log.print(L"Service already stopped: " + std::wstring{ G_SERVICE_NAME });
            return true;
        }

        // Останавливаем
        if (!ControlService(svc, SERVICE_CONTROL_STOP, &ss))
        {
            const DWORD err = GetLastError();
            if (err == ERROR_SERVICE_NOT_ACTIVE)
            {
                log.print(L"Service already stopped: " + std::wstring{ G_SERVICE_NAME });
                return true;
            }
            log.printWarn(L"ControlService(STOP) failed, error=" + std::to_wstring(err));
            return false;
        }

        // ждем пока сервис остановится, максимум 5сек (50 тактов по 100мс)
        for (int i = 0; i < 50; ++i)
        {
            if (!QueryServiceStatus(svc, &ss))
                break;
            if (ss.dwCurrentState == SERVICE_STOPPED)
                break;
            Sleep(100);
        }

        if (ss.dwCurrentState == SERVICE_STOPPED)
        {
            log.print(L"Service stopped successfully!");
            return true;
        }

        log.printWarn(L"Service did not reach STOPPED, state=" + std::to_wstring(ss.dwCurrentState));
        return false;
    }

    //------------------

    void runServiceBody()
    {
        std::shared_ptr<kas::utils::logger::FileLogger> log_uptr{ nullptr };

        const auto log_path_opt{ makeServiceLogPath() };
        if (log_path_opt)
        {
            log_uptr = std::make_unique<kas::utils::logger::FileLogger>(log_path_opt.value());
        }
        if (log_uptr)
        {
            log_uptr->print(L"service started\n");
            log_uptr->flush();
        }

        try
        {
            net::WinsockGuard _wsg{};

            auto server = std::make_unique<net::TcpServer>(G_SERVER_PORT, log_uptr);
            G_SERVER.store(server.get(), std::memory_order_release);

            std::jthread _server_thread([&] {
                while (!G_STOP_EVENT.load(std::memory_order_acquire))
                {
                    if (!server->run())
                    {
                        if (log_uptr)
                            log_uptr->printWarn(L"Can not run server on port:", G_SERVER_PORT);
                        Sleep(3000);
                    }
                }
                });

            while (!G_STOP_EVENT.load(std::memory_order_acquire))
            {
                Sleep(100);
            }

            server->stop();

            G_SERVER.store(nullptr, std::memory_order_release);
        }
        catch (const std::exception& ex)
        {
            if (log_uptr)
                log_uptr->printException(utils::helper::toWide(ex.what()));
        }
        catch (...)
        {
            if (log_uptr)
                log_uptr->printException(std::wstring(L"Unknown exception!"));
        }

        if (log_uptr)
        {
            log_uptr->print(L"service stopped\n");
            log_uptr->flush();
        }
    }

    //------------------
}