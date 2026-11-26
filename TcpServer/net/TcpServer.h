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
		TcpServer(std::wstring port, std::shared_ptr<kas::utils::logger::FileLogger> log);

		TcpServer(const TcpServer&) = delete;
		TcpServer(TcpServer&&) noexcept = delete;
		TcpServer& operator=(const TcpServer&) = delete;
		TcpServer& operator=(TcpServer&&) noexcept = delete;
		~TcpServer();

		[[nodiscard]] bool run();

		void stop();

	private:
		net::SocketScoped m_listen{ INVALID_SOCKET };
		std::atomic_bool  m_running{ false };
		std::shared_ptr<kas::utils::logger::FileLogger> m_log_shptr;
	};
}