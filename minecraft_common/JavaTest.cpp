#include "JavaTest.h"
#include "base/StringHelper.h"
#include "base/PathService.h"
#include "glog/logging.h"
#include "base/at_exit_manager.h"
#include "base/ResourceHelper.h"

#include <windows.h>
#include <shlobj.h>
#include <atlstr.h>
#include <atlconv.h>
#include <string>
#include <codecvt>
#include <algorithm>

JavaTest::JavaTest()
{
	static std::wstring path = JavaTest::extract_test_jar();
	jar_path_ = path;
}


JavaTest::~JavaTest()
{
}

bool JavaTest::get_java_version(const std::wstring &java_path, std::string &ver, bool &is_x64)
{
	SECURITY_ATTRIBUTES sa;
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = nullptr;
	sa.nLength = sizeof(sa);

	HANDLE read = nullptr;
	HANDLE write = nullptr;
	if (!::CreatePipe(&read, &write, &sa, 0))
	{
		DCHECK(0);
		return false;
	}
	::SetHandleInformation(read, HANDLE_FLAG_INHERIT, 0);

	AtExitManager at_exit;
	at_exit.register_callback(make_closure([read, write]()
	{
		if(read)
			::CloseHandle(read);
		if(write)
			::CloseHandle(write);
	}));


	std::wstring cmd = L"\"" + java_path + L"\" -jar \"" + jar_path_ + L"\"";

	STARTUPINFO startup_info = { 0 };
	startup_info.cb = sizeof(startup_info);
	startup_info.wShowWindow = SW_HIDE;
	startup_info.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	startup_info.hStdOutput = write;

	PROCESS_INFORMATION pi;

	if (!::CreateProcessW(nullptr, (LPWSTR)cmd.c_str(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &startup_info, &pi))
	{
		DWORD err = GetLastError();
		std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
		LOG(ERROR) << " create process with " << converter.to_bytes(cmd) << "failed with err " << err;
		DCHECK(0);
		return false;
	}

	::CloseHandle(pi.hThread);
	::WaitForSingleObject(pi.hProcess, 4000);

	HANDLE process = pi.hProcess;
	at_exit.register_callback(make_closure([process]()
	{
		::CloseHandle(process);
	}));

	char buf[128] = { 0 };
	DWORD avail = 0;
	DWORD read_size = 0;
	if (0 != PeekNamedPipe(read, NULL, 0, NULL, &avail, NULL) && avail > 0)
	{
		::ReadFile(read, (LPVOID)buf, (_countof(buf) - 1) * sizeof(char), &read_size, nullptr);
		char* ptr = strstr(buf, "||");
		if (read_size < 4 || ptr == nullptr || buf == ptr || ptr - buf == read_size - strlen("||"))
			return false;
		else
		{
			is_x64 = (stricmp(ptr + 2, "x86") != 0);

			if (ptr - buf < 3)
				return false;

			ver.assign(buf, ptr - buf);

			return true;
		}
	}
	else
	{
		return false;
	}	
}

std::wstring JavaTest::extract_test_jar()
{
	//资源id是从zuimc_launcher获取的，要是改了的话，记得这里也变一下
#ifndef IDR_FILE_JAVA_CHECKER
#define IDR_FILE_JAVA_CHECKER (101)
#endif

	std::wstring guid_string;
	guid_wstring(guid_string);

	std::wstring extrace_jar_path = PathService().appdata_path();
	extrace_jar_path += L"\\temp\\";
	::SHCreateDirectory(0, extrace_jar_path.c_str());
	extrace_jar_path += guid_string;
	extrace_jar_path += L".jar";

	if (!extrace_resource(GetModuleHandle(nullptr), MAKEINTRESOURCEW(IDR_FILE_JAVA_CHECKER), L"FILE", extrace_jar_path))
	{
		auto err = GetLastError();
		std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
		LOG(ERROR) << "extrace " << converter.to_bytes(extrace_jar_path).c_str() << "failed. err: " << err;
		DCHECK(0);
		return L"";
	}
	//锁住文件
	static HANDLE file_lock = CreateFileW(extrace_jar_path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);

	return extrace_jar_path;
}
