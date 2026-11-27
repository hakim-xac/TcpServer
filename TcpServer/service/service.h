#pragma once

#include <string>

namespace kas::service
{
	// Пытается запуститься как служба через SCM.
	// true - диспетчер служб принял процесс; false - запуск не как службы.
	[[nodiscard]] bool tryRunAsService();

	// Устанавливает службу в SCM (создает запись, автозапуск).
	[[nodiscard]] bool installService();

	// Удаляет службу из SCM: останавливает (если запущена) и вызывает DeleteService.
	[[nodiscard]] bool unInstallService();

	// Запускает службу и ждет до 5 сек перехода в SERVICE_RUNNING.
	[[nodiscard]] bool startService();

	// Останавливает службу и ждет до 5 сек перехода в SERVICE_STOPPED.
	[[nodiscard]] bool stopService();

	// Основная работа службы: лог, Winsock, TcpServer в отдельном потоке,
	// ожидание флага остановки и корректное завершение.
	void runServiceBody();
}