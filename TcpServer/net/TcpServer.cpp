#include "TcpServer.h"
#include "../utils/helper.h"

#include <winsock2.h>

#include <filesystem>
#include <string>
#include <memory>

namespace kas::net
{
	namespace
	{
		constexpr std::size_t MAX_DATA_SIZE_LIMIT{ 10 * 1024 * 1024 }; // 10MB
		//------------------
		
		// Читает ровно len байт из сокета в buf; false при обрыве/ошибке.
		bool recvAll(SOCKET s, char* buf, int len)
		{
			int total = 0;
			while (total < len) {
				const int n = recv(s, buf + total, len - total, 0);
				if (n <= 0)
					return false;
				total += n;
			}
			return true;
		}

		//------------------

		// Отправляет ровно len байт из buf; false при ошибке.
		bool sendAll(SOCKET s, const char* buf, int len)
		{
			int total = 0;
			while (total < len) {
				const int n = send(s, buf + total, len - total, 0);
				if (n == SOCKET_ERROR)
					return false;

				total += n;
			}
			return true;
		}

		//------------------

		// Читает один пакет: 4 байта длины (big-endian) + тело.
		// Возвращает nullopt при ошибке, пустом теле или превышении лимита 'MAX_DATA_SIZE_LIMIT'.
		[[nodiscard]] std::optional<std::string>
		readPacket(SOCKET s)
		{
			std::uint32_t len_be{};
			if (!recvAll(s, reinterpret_cast<char*>(&len_be), sizeof(len_be)))
				return std::nullopt;

			const std::uint32_t len = ntohl(len_be);
			if (len == 0 || len > MAX_DATA_SIZE_LIMIT)
				return std::nullopt;

			std::string res (len, '\0');

			if (!recvAll(s, res.data(), static_cast<int>(len)))
				return std::nullopt;

			return res;
		}

		//------------------
		
		// Отправляет один пакет: 4 байта длины (big-endian) + тело.
		[[nodiscard]] bool 
		writePacket(SOCKET s, const std::string& json)
		{
			const std::uint32_t len_be = htonl(static_cast<std::uint32_t>(json.size()));
			if (!sendAll(s, reinterpret_cast<const char*>(&len_be), sizeof(len_be)))
				return false;
			return sendAll(s, json.data(), static_cast<int>(json.size()));
		}

		//------------------
		
		// Извлекает строковое значение поля "requiredDirectory" из JSON-текста
		// вручную (без парсера), с обработкой escape-последовательностей.
		[[nodiscard]] std::optional<std::string>
		extractRequiredDirectory(const std::string& json)
		{
			const std::string key = "\"requiredDirectory\"";
			const auto key_pos = json.find(key);
			if (key_pos == std::string::npos)
				return std::nullopt;

			const auto colon{ json.find(':', key_pos + key.size()) };
			if (colon == std::string::npos)
				return std::nullopt;

			const auto quote1{ json.find('"', colon + 1) };
			if (quote1 == std::string::npos)
				return std::nullopt;

			std::string result;
			for (std::size_t i = quote1 + 1; i < json.size(); ++i)
			{
				const char c = json[i];
				if (c == '\\' && i + 1 < json.size()) {
					const char next = json[++i];
					switch (next) {
					case '"':  result += '"';  break;
					case '\\': result += '\\'; break;
					case '/':  result += '/';  break;
					case 'n':  result += '\n'; break;
					case 't':  result += '\t'; break;
					case 'r':  result += '\r'; break;
					default:   result += next; break;
					}
				}
				else if (c == '"')
				{
					if (result.empty())
						return std::nullopt;
					return result;
				}
				else
					result += c;
			}
			return std::nullopt;
		}

		//------------------
		
		// Экранирует строку для вставки в JSON-литерал.
		[[nodiscard]] std::string
		jsonEscape(std::string s)
		{
			std::string out;
			out.reserve(s.size() + 8);
			for (char c : s) {
				switch (c) {
				case '"':  out += "\\\""; break;
				case '\\': out += "\\\\"; break;
				case '\n': out += "\\n";  break;
				case '\r': out += "\\r";  break;
				case '\t': out += "\\t";  break;
				default:   out += c;      break;
				}
			}
			return out;
		}

		//------------------
		
		// Формирует JSON-ответ с ошибкой: {"status":false,"text_error":"..."}.
		[[nodiscard]] std::string
		makeErrorJson(std::string text)
		{
			return "{\"status\":false,\"text_error\":\"" + jsonEscape(std::move(text)) + "\"}";
		}

		//------------------
		
		// Преобразует входной путь в абсолютный: абсолютный оставляем,
		// относительный считаем от каталога exe; нормализует через weakly_canonical.
		[[nodiscard]] std::optional<std::filesystem::path>
		resolvePath(const std::string& raw)
		{
			namespace fs = std::filesystem;

			if (raw.empty())
				return std::nullopt;

			fs::path p;

			// Если начинается с '\' или '/' считаем, что путь абсолютный
			if (raw.front() == '\\' || raw.front() == '/')
				p = fs::path{ raw };
			else
				p = utils::helper::getExeDirectory() / fs::path{ raw };

			std::error_code ec;
			p = fs::weakly_canonical(p, ec);
			if (ec)
				p = p.lexically_normal();

			return p;
		}

		//------------------
		
		// Обрабатывает запрос "список файлов в каталоге": проверяет путь
		// и возвращает JSON со списком entries (name, type).
		[[nodiscard]] std::string
		handleListDirectory(const std::string& path)
		{
			namespace fs = std::filesystem;

			if (path.empty())
				return makeErrorJson("Directory path is empty");

			std::error_code ec;
			const auto p_opt{ resolvePath(path) };
			if (!p_opt) [[unlikely]]
				return makeErrorJson("Invalid directory path: " + path);

			const auto& p{ p_opt.value() };

			if (p.empty())
				return makeErrorJson("Invalid directory path: " + path);

			if (!fs::exists(p, ec) || ec)
				return makeErrorJson("Directory not found: " + path);

			if (!fs::is_directory(p, ec) || ec)
				return makeErrorJson("Not a directory: " + path);

			std::string entries = "[";
			bool first = true;

			for (const auto& entry : fs::directory_iterator( p, fs::directory_options::skip_permission_denied, ec))
			{
				if (ec)
					break;

				const bool is_dir = entry.is_directory(ec);
				if (ec)
					continue;

				if (!first)
					entries += ",";
				first = false;

				auto name_u8{ entry.path().filename().u8string() };
				std::string name (name_u8.begin(), name_u8.end());

				entries += "{\"name\":\"" + jsonEscape(std::move(name)) + "\",\"type\":\"";
				entries += (is_dir ? "directory" : "file");
				entries += "\"}";
			}
			entries += "]";

			return "{\"status\":true,\"entries\":" + entries + "}";
		}

		//------------------
		
		// Цикл обработки одного клиента: читает пакеты и отвечает на каждый
		// запрос списком каталога или JSON-ошибкой.
		void handleClient(SOCKET client, std::shared_ptr<kas::utils::logger::FileLogger> log)
		{
			while (const auto packet_opt{ readPacket(client) })
			{
				const auto response{
					[&packet_opt] {
						const auto dir_opt { extractRequiredDirectory(packet_opt.value()) };
						return dir_opt
							? handleListDirectory(dir_opt.value())
							: makeErrorJson("Field 'requiredDirectory' is missing or empty");
					}()
				};

				if (!writePacket(client, response))
					break;
			}
		}

		//------------------
	}

	
	//------------------
	
	TcpServer::TcpServer(std::wstring port, std::shared_ptr<kas::utils::logger::FileLogger> log)
		: m_log_shptr{ std::move(log) }
	{
		ADDRINFOW hints{};
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_PASSIVE;

		PADDRINFOW result = nullptr;
		int rc = GetAddrInfoW(nullptr, port.c_str(), &hints, &result);
		if (rc != 0)
			throw std::runtime_error("GetAddrInfoW failed: " + std::to_string(rc));

		struct AddrInfoGuard
		{
			PADDRINFOW p;
			~AddrInfoGuard() { if (p) FreeAddrInfoW(p); }
		} result_guard{ result };

		SOCKET listen_sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (listen_sock == INVALID_SOCKET)
			throw std::runtime_error("socket failed: " + std::to_string(WSAGetLastError()));

		net::SocketScoped listen_sock_scoped{ listen_sock };

		BOOL yes = TRUE;
		setsockopt(listen_sock_scoped.getSocketID(), SOL_SOCKET, SO_REUSEADDR,
			reinterpret_cast<const char*>(&yes), sizeof(yes));

		if (bind(listen_sock_scoped.getSocketID(), result->ai_addr, static_cast<int>(result->ai_addrlen)) == SOCKET_ERROR)
		{
			const int err = WSAGetLastError();
			throw std::runtime_error("bind failed: " + std::to_string(err));
		}

		if (listen(listen_sock_scoped.getSocketID(), SOMAXCONN) == SOCKET_ERROR)
		{
			const int err = WSAGetLastError();
			throw std::runtime_error("listen failed: " + std::to_string(err));
		}

		if (m_log_shptr)
		{
			m_log_shptr->printInfo(L"Listen port:", port);
			m_log_shptr->flush();
		}

		m_listen = std::move(listen_sock_scoped);
	}

	//------------------
	
	TcpServer::~TcpServer()
	{
		stop();
	}

	//------------------
	
	bool TcpServer::run()
	{
		try
		{
			if (!m_listen.isValid())
			{
				if (m_log_shptr) [[unlikely]]
					m_log_shptr->printWarn(L"TcpServer::run: listen socket is invalid");

				return false;
			}

			m_running = true;
			while (m_running)
			{
				sockaddr_storage addr{};
				int addrLen = sizeof(addr);

				// сокет блокирующий
				SOCKET client = accept(m_listen.getSocketID(),
					reinterpret_cast<sockaddr*>(&addr), &addrLen);
				if (client == INVALID_SOCKET)
				{
					if (!m_running) break;

					const int err = WSAGetLastError();

					if (err == WSAENOTSOCK || err == WSAEINVAL)
					{
						if (m_log_shptr)
							m_log_shptr->printError(L"accept failed: " + std::to_wstring(err));
						break;
					}

					Sleep(100);
					continue;
				}

				// самый простой вариант. На каждого клиента вызывать свой std::thread
				auto log = m_log_shptr;
				std::thread([client, log = std::move(log)] {
					net::SocketScoped clientGuard{ client };
					handleClient(clientGuard.getSocketID(), log);
					}).detach();
			}
		}
		catch (const std::exception& ex)
		{
			if (m_log_shptr)
				m_log_shptr->printException(utils::helper::toWide(ex.what()));
			return false;
		}
		catch (...)
		{
			if (m_log_shptr)
				m_log_shptr->printException(L"Unckown Exception!");
			return false;
		}
		return true;
	}

	//------------------
	
	void TcpServer::stop()
	{
		m_running = false;
		m_listen.reset();
	}

	//------------------
}