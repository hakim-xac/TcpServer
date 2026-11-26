#include <iostream>

#include "utils/utils.h"
#include "console/console.h"
#include "service/service.h"

#include <string>

int wmain(int argc, wchar_t* argv[])
{
	if (kas::service::tryRunAsService())
		return 0;

	kas::utils::args::ArgsHandler args_handler{ argc, argv , std::wcout };
	kas::console::initListSupportedCommands(args_handler);
	return args_handler.call();
}