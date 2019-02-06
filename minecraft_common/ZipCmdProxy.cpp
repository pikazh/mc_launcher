#include "ZipCmdProxy.h"
#include "base/StringHelper.h"
#include "base/PathService.h"
#include "glog/logging.h"
#include "base/ResourceHelper.h"

#include <windows.h>
#include <shlobj.h>
#include <atlstr.h>
#include <atlconv.h>
#include <string>
#include <codecvt>

ZipCmdProxy::ZipCmdProxy()
{
	static std::wstring zip = ZipCmdProxy::uncompress_zip();
	g_uncompress_zip = zip;
}

ZipCmdProxy::~ZipCmdProxy()
{
}

int ZipCmdProxy::run(const std::wstring &param)
{
	std::wstring cmd = L"\"" + g_uncompress_zip + L"\" " + param;

	STARTUPINFO startup_info = { 0 };
	startup_info.cb = sizeof(startup_info);
	startup_info.wShowWindow = SW_HIDE;
	startup_info.dwFlags = STARTF_USESHOWWINDOW;

	PROCESS_INFORMATION pi;
	if (!::CreateProcessW(nullptr, (LPWSTR)cmd.c_str(), nullptr, nullptr, false, 0, nullptr, nullptr, &startup_info, &pi))
	{
		auto err = ::GetLastError();
		USES_CONVERSION;
		std::string c = ATL::CW2A(cmd.c_str());
		LOG(ERROR) << "create process with " << c << " failed with err: " << err;
		return err;
	}

	CloseHandle(pi.hThread);
	WaitForSingleObject(pi.hProcess, INFINITE);
	DWORD exit_code = 0;
	::GetExitCodeProcess(pi.hProcess, &exit_code);
	CloseHandle(pi.hProcess);
	return exit_code;
}

std::wstring ZipCmdProxy::uncompress_zip()
{
	//资源id是从zuimc_launcher获取的，要是改了的话，记得这里也变一下
#ifndef IDR_FILE_7Z
#define IDR_FILE_7Z (105)
#endif
#ifndef IDR_FILE_7Z_DLL
#define IDR_FILE_7Z_DLL (106)
#endif
	std::wstring guid;
	guid_wstring(guid);
	std::wstring extrace_path;

	extrace_path = PathService().appdata_path();
	extrace_path += L"\\temp\\" + guid + L"\\7z.dll";
	if (!extrace_resource(GetModuleHandle(nullptr), MAKEINTRESOURCEW(IDR_FILE_7Z_DLL), L"FILE", extrace_path))
	{
		auto err = GetLastError();
		std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
		LOG(ERROR) << "extrace " << converter.to_bytes(extrace_path).c_str() << "failed. err: " << err;
		DCHECK(0);
		return L"";
	}
	//锁住文件
	static HANDLE dll_file_lock = CreateFileW(extrace_path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);


	extrace_path = PathService().appdata_path();
	extrace_path += L"\\temp\\" + guid;
	extrace_path += L"\\";
	extrace_path += guid;
	extrace_path += L".exe";

	if (!extrace_resource(GetModuleHandle(nullptr), MAKEINTRESOURCEW(IDR_FILE_7Z), L"FILE", extrace_path))
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
		LOG(ERROR) << "extrace " << converter.to_bytes(extrace_path).c_str() << "failed. err: " << GetLastError();
		DCHECK(0);
		return L"";
	}
	//锁住文件
	static HANDLE zip_file_lock = CreateFileW(extrace_path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);

	return extrace_path;
}

