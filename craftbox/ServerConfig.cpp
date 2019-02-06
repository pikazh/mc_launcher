#include "ServerConfig.h"
#include "base\PathService.h"

#include <memory>
#include <windows.h>

ServerConfig* ServerConfig::global()
{
	static std::shared_ptr<ServerConfig> global = std::make_shared<ServerConfig>();
	return global.get();
}

ServerConfig::ServerConfig()
{
	ini_path = PathService().exe_dir() + L"\\craftbox.ini";
}


ServerConfig::~ServerConfig()
{
}

uint32_t ServerConfig::sid()
{
	return ::GetPrivateProfileIntW(L"craftbox", L"sid", 0, ini_path.c_str());
}
