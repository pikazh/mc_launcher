#pragma once

#include <windows.h>
#include <string>

class FileVersion
{
public:
	bool get_file_version(const wchar_t* file_path_u16, LPDWORD main_ver, LPDWORD sub_ver, LPDWORD build_number, LPDWORD revision);
	std::string get_file_version(const wchar_t* file_path_u16);
	static std::string app_file_version();

protected:
	static std::string get_app_file_version();
private:
	static std::string g_app_version;
};

