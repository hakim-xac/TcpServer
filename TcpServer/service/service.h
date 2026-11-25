#pragma once

#include <string>

namespace kas::service
{
	[[nodiscard]] bool tryRunAsService();

	[[nodiscard]] bool installService();

	[[nodiscard]] bool unInstallService(const std::wstring& exe_path);

	void runServiceBody();
}