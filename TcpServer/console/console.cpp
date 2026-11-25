#include "console.h"
#include "../service/service.h"
#include "../utils/utils.h"

namespace kas::console
{
	namespace
	{
		//------------------

		bool runAsDaemon()
		{
			kas::utils::logger::LoggerSync log{};
			log.print(L"run as daemon");
			return true;
		}

		//------------------
	}

	//------------------

	void initListSupportedCommands(kas::utils::args::ArgsHandler& args_handler)
	{
		args_handler.insert(L"--daemon", runAsDaemon);
		args_handler.insert(L"--install-service", []() -> bool {

			const bool install_succes{ service::installService() };
			kas::utils::logger::LoggerSync{}.print(L"install_succes:", install_succes);
			return install_succes;
		});

		args_handler.insert(L"--uninstall-service", []() -> bool {

			std::wstring path{ L"test" };
			return service::unInstallService(path);
		});
	}

	//------------------


}