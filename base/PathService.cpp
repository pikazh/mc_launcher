#include "PathService.h"

#include <shlobj.h>
#include <shlwapi.h>

std::wstring PathService::appdata_path()
{
	wchar_t path[1024] = { 0 };
	::SHGetFolderPathW(0, CSIDL_APPDATA, 0, 0, path);
	wcscat(path, L"\\craftbox");
	return path;
}

std::wstring PathService::temp_path()
{
	wchar_t path[1024] = { 0 };
	::GetTempPathW(_countof(path), path);
	return path;
}

std::wstring PathService::exe_dir()
{
	wchar_t path[1024] = { 0 };
	::GetModuleFileNameW(nullptr, path, 1024);
	*(::PathFindFileNameW(path)-1) = 0;
	return path;
}
