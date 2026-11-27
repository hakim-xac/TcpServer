#include <iostream>

#include "utils/utils.h"
#include "console/console.h"
#include "service/service.h"

#include <string>

int wmain(int argc, wchar_t* argv[])
{
	// Система пробует запустить приложение как сервис.
	if (kas::service::tryRunAsService())
		return 0;

	// Запуск из консоли.
	kas::utils::args::ArgsHandler args_handler{ argc, argv , std::wcout };

	// Регистрирует поддерживаемые ключи командной строки и их обработчики.
	kas::console::initListSupportedCommands(args_handler);

	// выполняем команду
	return args_handler.call();
}