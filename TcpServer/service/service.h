#pragma once

#include <string>

namespace kas::service
{
	[[nodiscard]] bool tryRunAsService();

	[[nodiscard]] bool installService();

	[[nodiscard]] bool unInstallService();

	[[nodiscard]] bool startService();

	[[nodiscard]] bool stopService();

	void runServiceBody();
}