#pragma once

#include <winsock2.h> 

#include <utility> 

namespace kas::net
{
	class SocketScoped final {
	public:
		SocketScoped(SOCKET socket) :
			m_socket{ socket }
		{
		}

		SocketScoped(const SocketScoped&) = delete;
		SocketScoped(SocketScoped&& other) noexcept :
			m_socket{ std::exchange(other.m_socket, INVALID_SOCKET) }
		{
		}
		SocketScoped& operator=(const SocketScoped&) = delete;
		SocketScoped& operator=(SocketScoped&& other) noexcept
		{
			if (std::addressof(other) != this)
			{
				std::swap(other.m_socket, m_socket);
			}
			return *this;
		}

		~SocketScoped()
		{
			if (m_socket != NULL)
				closesocket(m_socket);
		}

		[[nodiscard]] inline bool isValid() const noexcept
		{
			return m_socket != INVALID_SOCKET;
		}

		inline void reset(SOCKET socket = INVALID_SOCKET) noexcept
		{
			if (m_socket != INVALID_SOCKET)
			{
				closesocket(m_socket);
			}
			m_socket = socket;
		}

		[[nodiscard]] inline SOCKET release() noexcept
		{
			return std::exchange(m_socket, INVALID_SOCKET);
		}

		[[nodiscard]] inline SOCKET getSocketID() const noexcept
		{
			return m_socket;
		}

	private:
		SOCKET m_socket;
	};
}