#pragma once
#pragma comment(lib, "Ws2_32.lib")

#include <winsock2.h>
#include <ws2tcpip.h>
#include "WinsockGuard.h"
#include "SocketScoped.h"
#include "../utils/utils.h"

#include <stdexcept>
#include <string>
#include <memory>
#include <atomic>
#include <thread>

namespace kas::net
{
	class TcpServer final
	{
	public:

		// Конструктор: резолвит адрес, создает, настраивает и переводит в listen
		// слушающий сокет; бросает исключение при ошибке.
		TcpServer(std::wstring port, std::shared_ptr<kas::utils::logger::FileLogger> log);

		// Запрещает копирование и перемещение.
		TcpServer(const TcpServer&) = delete;
		TcpServer(TcpServer&&) noexcept = delete;
		TcpServer& operator=(const TcpServer&) = delete;
		TcpServer& operator=(TcpServer&&) noexcept = delete;

		// Деструктор: гарантирует остановку сервера.
		~TcpServer();

		// Основной цикл accept: на каждого клиента создает отдельный detached-поток. (самое простое решение)
		// Возвращает false при фатальной ошибке или если listen-сокет невалиден.
		[[nodiscard]] bool run();

		// Останавливает сервер: сбрасывает флаг и закрывает слушающий сокет,
		// чтобы разблокировать accept.
		void stop();

	private:
		net::SocketScoped m_listen{ INVALID_SOCKET };
		std::atomic_bool  m_running{ false };
		std::shared_ptr<kas::utils::logger::FileLogger> m_log_shptr;
	};
}