#include "console.h"
#include "../service/service.h"
#include "../utils/utils.h"
#include "../net/net.h"
#include "../GlobalVariables.h"

#include <stdexcept>
#include <shlobj.h>
#include <atomic>
#include <memory>

namespace kas::console
{
	namespace
	{
		std::atomic_bool G_STOP_EVENT{};
		std::atomic<kas::net::TcpServer*> G_SERVER{ nullptr };

		//=========================================
		
		// Обработчик консольных событий (Ctrl+C/Break/Close/Shutdown):
		// выставляет флаг остановки и просит сервер завершиться.
		BOOL WINAPI consoleCtrlHandler(DWORD ctrlType)
		{
			switch (ctrlType)
			{
			case CTRL_C_EVENT:
			case CTRL_BREAK_EVENT:
			case CTRL_CLOSE_EVENT:
			case CTRL_SHUTDOWN_EVENT:
			{
				G_STOP_EVENT.store(true, std::memory_order_release);
				if (auto* srv = G_SERVER.load(std::memory_order_acquire))
					srv->stop();

				return TRUE;
			}
			default:
				return FALSE;
			}
		}

		//------------------

		// Формирует путь к лог-файлу демона в %ProgramData%\Kas\TcpServer\logs,
		// создавая каталог при необходимости.
		[[nodiscard]] std::optional<std::wstring>
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

			return dir + L"\\TcpServerAsDaemon.log";
		}

		//------------------
		
		// Возвращает порт из аргумента --server_port или значение по умолчанию.
		[[nodiscard]] std::wstring_view
		getServerPort(kas::utils::args::ArgsHandler& args_handler)
		{
			const auto port_param_opt { args_handler.getInputArgs().getKey(L"--server_port") };
			return port_param_opt ? port_param_opt.value() : G_SERVER_PORT;
		}
		
		//------------------

		// Запускает сервер в режиме демона: лог, Winsock, TcpServer в отдельном
		// потоке, ожидание флага остановки и корректное завершение.
		[[nodiscard]] bool
		runAsDaemon(kas::utils::args::ArgsHandler& args_handler)
		{
			std::shared_ptr<kas::utils::logger::FileLogger> log_uptr{ nullptr };

			const auto log_path_opt{ makeServiceLogPath() };
			if (log_path_opt)
			{
				log_uptr = std::make_shared<kas::utils::logger::FileLogger>(log_path_opt.value());
			}

			try
			{
				net::WinsockGuard _wsg{};

				const std::wstring server_port { getServerPort(args_handler) };

				auto server = std::make_unique<net::TcpServer>(server_port, log_uptr);
				G_SERVER.store(server.get(), std::memory_order_release);

				std::jthread _main_th{
					[&] {
					while (!G_STOP_EVENT.load(std::memory_order_acquire))
						if (!server->run())
						{
							if (log_uptr)
								log_uptr->printWarn(L"Can not run server on port:", server_port);
							Sleep(3000);
						}
					}
				};

				while (!G_STOP_EVENT.load(std::memory_order_acquire))
					Sleep(100);

				server->stop();

				if (log_uptr)
					log_uptr->printInfo(L"KasTcpServerAsDaemon stopped!");

				G_SERVER.store(nullptr, std::memory_order_release);
			}
			catch (const std::exception& ex)
			{
				if (log_uptr)
					log_uptr->printException(utils::helper::toWide(ex.what()));
				return false;
			}
			catch (...)
			{
			if (log_uptr)
					log_uptr->printException("Unknown exception!");
				return false;
			}
			
			return true;
		}

		//------------------
	}

	//------------------

	void initListSupportedCommands(kas::utils::args::ArgsHandler& args_handler)
	{
		args_handler.insert(L"--daemon", [&args_handler] {
			return runAsDaemon(args_handler);
		})
			.setDescription(L"Runs the program as a daemon");

		args_handler.insert(L"--stop-service", service::stopService)
			.setDescription(L"Stops the service");

		args_handler.insert(L"--start-service", service::startService)
			.setDescription(L"Starts the service");

		args_handler.insert(L"--install-service", service::installService)
			.setDescription(L"Installs the service into the system");

		args_handler.insert(L"--uninstall-service", service::unInstallService)
			.setDescription(L"Removes the service from the system");
	}

	//------------------


}