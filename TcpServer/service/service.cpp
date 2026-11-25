#include "service.h"
#include "../utils/utils.h"

#include <windows.h>
#include <string>

namespace kas::service
{
    namespace
    {
        static SERVICE_STATUS G_SERVICE_STATUS{};
        static SERVICE_STATUS_HANDLE G_SERVICE_STATUS_HANDLE{};
        static HANDLE G_STOP_EVENT{ nullptr };

        constexpr wchar_t kServiceName[] = L"KasService";
        constexpr wchar_t kServiceDisplayName[] = L"Kas Service";

        //=========================================

        void WINAPI handler(DWORD ctrl)
        {
            switch (ctrl) {
            case SERVICE_CONTROL_STOP:
            case SERVICE_CONTROL_SHUTDOWN:
                G_SERVICE_STATUS.dwCurrentState = SERVICE_STOP_PENDING;
                SetServiceStatus(G_SERVICE_STATUS_HANDLE, &G_SERVICE_STATUS);
                if (G_STOP_EVENT)
                    SetEvent(G_STOP_EVENT);
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

            G_STOP_EVENT = CreateEventW(nullptr, TRUE, FALSE, nullptr);
            if (!G_STOP_EVENT)
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

            CloseHandle(G_STOP_EVENT);
            G_STOP_EVENT = nullptr;
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
    }

    //------------------
        
    bool tryRunAsService()
    {
        SERVICE_TABLE_ENTRYW table[] = {
            { const_cast<LPWSTR>(kServiceName), serviceMain },
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
            log.print(L"Unable carrent exe path!");
            return false;
        }

        SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
        if (!scm) [[unlikely]]
        {
            DWORD err = GetLastError();
            if (err == ERROR_ACCESS_DENIED)
                log.print(L"Access denied: run as Administrator");
            else
                log.print(L"OpenSCManagerW failed, error=" + std::to_wstring(err));
            return false;
        }

        SC_HANDLE svc = CreateServiceW(
            scm,
            kServiceName,
            kServiceDisplayName,
            SERVICE_ALL_ACCESS,
            SERVICE_WIN32_OWN_PROCESS,
            SERVICE_AUTO_START,
            SERVICE_ERROR_NORMAL,
            path_opt->c_str(),
            nullptr, nullptr, nullptr, nullptr, nullptr);

        bool ok = (svc != nullptr);
        DWORD err = ok ? ERROR_SUCCESS : GetLastError();

        if (svc)
            CloseServiceHandle(svc);
        CloseServiceHandle(scm);

        if (!ok && err == ERROR_SERVICE_EXISTS)
            return true;

        return ok;
    }

    //------------------

    bool unInstallService(const std::wstring& exe_path)
    {
        SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
        if (!scm)
            return false;

        SC_HANDLE svc = OpenServiceW(scm, kServiceName, SERVICE_STOP | DELETE);
        if (!svc)
        {
            DWORD err = GetLastError();
            CloseServiceHandle(scm);
            return err == ERROR_SERVICE_DOES_NOT_EXIST;
        }

        SERVICE_STATUS status{};
        if (ControlService(svc, SERVICE_CONTROL_STOP, &status))
        {
            for (int i = 0; i < 100; ++i)
            {
                if (!QueryServiceStatus(svc, &status))
                    break;
                if (status.dwCurrentState == SERVICE_STOPPED)
                    break;
                Sleep(100);
            }
        }

        bool ok = DeleteService(svc) != FALSE;
        DWORD err = ok ? ERROR_SUCCESS : GetLastError();

        CloseServiceHandle(svc);
        CloseServiceHandle(scm);

        if (!ok && err == ERROR_SERVICE_MARKED_FOR_DELETE)
            return true;

        return ok;
    }

    //------------------
    
    void runServiceBody()
    {
        kas::utils::logger::LoggerSync log{};
        log.print(L"service started");

        if (G_STOP_EVENT)
            WaitForSingleObject(G_STOP_EVENT, INFINITE);

        log.print(L"service stopped");
    }
    
    //------------------
	
}