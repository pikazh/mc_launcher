#pragma once

#include <windows.h>
#include <string>

#define IPC_MAGIC_NUM	(198738)

#define IPC_MSG_LOGIN	(1000)

#pragma pack(push)
#pragma pack(1)

struct IPCDATA
{
	UINT32 msg;
	std::string data;
};

#pragma pack(pop)
