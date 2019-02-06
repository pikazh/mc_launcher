#pragma once

#include <string>

class ServerConfig
{
public:
	static ServerConfig* global();
	ServerConfig();
	virtual ~ServerConfig();

	uint32_t sid();

private:
	std::wstring ini_path;
};

