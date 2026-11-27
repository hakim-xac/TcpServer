#pragma once

#include "../utils/args/ArgsHandler.h"

namespace kas::console
{
	// Регистрирует поддерживаемые ключи командной строки и их обработчики.
	void initListSupportedCommands(kas::utils::args::ArgsHandler& args_handler);
}