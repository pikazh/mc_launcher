#include "main.h"
#include "glog/logging.h"
#include "base/PathService.h"
#include "base/system_info.h"
#include "base/asynchronous_task_dispatch.h"
#include "base/NetworkTaskDispatcher.h"
#include "base/ResourceHelper.h"
#include "base/FileHelper.h"
#include "MainWnd.h"
#include "resource.h"
#include "AppConfig.h"
#include "AppUpdate.h"
#include "ui_helper.h"
#include "base/file_version.h"
#include "UpdateSuccessNotifyWnd.h"
#include "DataReport/DataReport.h"

#include <windows.h>
#include <shlobj.h>
#include <objbase.h>
#include <shlwapi.h>
#include <codecvt>
#include <atlbase.h>
#include <Strsafe.h>

Application::Application(HINSTANCE app_instance)
{
	::OleInitialize(nullptr);
	::CoInitialize(0);
	init_log();
	AsyncTaskDispatch::init_main_thread_dispatcher();
	CPaintManagerUI::SetInstance(app_instance);
}

Application::~Application()
{
	if (singleton_process_runnging_lock_)
	{
		CloseHandle(singleton_process_runnging_lock_);
		singleton_process_runnging_lock_ = nullptr;
	}

	if (singleton_process_exiting_share_memory_)
	{
		CloseHandle(singleton_process_exiting_share_memory_);
		singleton_process_exiting_share_memory_ = nullptr;
	}

	::CoUninitialize();
	::OleUninitialize();
}

int Application::run()
{
	if (!handle_singleton_process())
		return 1;

	DataReport::global()->report_event(APP_STARTUP_EVENT);

	EnableIEFeatures();
	if (!RegisterMCProtocol())
	{
		DataReport::global()->report_event(REGISTER_MC_PROTOCOL_FAILED_EVENT);
	}

	{
		char user_agent[2048] = { 0 };
		DWORD len = 0;
		UrlMkGetSessionOption(URLMON_OPTION_USERAGENT, user_agent, _countof(user_agent), &len, 0);
		user_agent[len] = 0;
		StringCchCatA(user_agent, _countof(user_agent), "; CraftBox ");
		
		std::string ua = user_agent + FileVersion::app_file_version();
		HRESULT ret = UrlMkSetSessionOption(URLMON_OPTION_USERAGENT, (LPVOID)ua.c_str(), ua.length(), 0);
		DCHECK(ret == S_OK);
	}

	DeleteFileW(std::wstring(PathService().exe_dir() + L"\\need_delete_file").c_str());

#ifndef _DEBUG
	std::shared_ptr<AppUpdate> app_update = std::make_shared<AppUpdate>();
	app_update->update_download_success.connect(this, &Application::OnDownloadUpdateSuccess);
	app_update->begin_update();
#endif
	AppConfig::global(); //强制初始化
	YggdrasilAuthentication::global();

	NetworkTaskDispatcher *network_task_dispatcher = NetworkTaskDispatcher::global();
	if (!network_task_dispatcher->start_schedule())
	{
		LOG(ERROR) << "start net dispatcher failed...";
		return 1;
	}

	std::thread extrace_sound_thread([]() 
	{
		std::wstring path = PathService().appdata_path() + L"\\dl_success.mp3";
		if (!::PathFileExistsW(path.c_str()) && !extrace_resource(GetModuleHandle(nullptr), MAKEINTRESOURCEW(IDR_SOUND_DL_SUCCESS), L"FILE", path))
		{
			auto err = GetLastError();
			std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
			LOG(ERROR) << "extrace " << converter.to_bytes(path).c_str() << "failed. err: " << err;
		}

		path = PathService().appdata_path() + L"\\dl_failed.mp3";
		if (!::PathFileExistsW(path.c_str()) && !extrace_resource(GetModuleHandle(nullptr), MAKEINTRESOURCEW(IDR_SOUND_DL_FAILED), L"FILE", path))
		{
			auto err = GetLastError();
			std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
			LOG(ERROR) << "extrace " << converter.to_bytes(path).c_str() << "failed. err: " << err;
		}
	});
	extrace_sound_thread.detach();

	auto wnd_obj = std::make_shared<MainWnd>();
	HWND main_wnd = wnd_obj->Create(0, mcfe::g_app_name, UI_WNDSTYLE_DIALOG|WS_MINIMIZEBOX, 0, 0, 0, 1024, 720);
	if (main_wnd == 0)
	{
		return 1;
	}
	wnd_obj->handle_command_line(GetCommandLineW());
	wnd_obj->SetIcon(IDI_ICON_LOGO);
	wnd_obj->CenterWindow();
	wnd_obj->ShowWindow();

	CPaintManagerUI::MessageLoop();

	wnd_obj.reset();

	signal_exiting();

	DataReport::global()->report_event(APP_EXIT_EVENT);

	network_task_dispatcher->stop_schedule();

	std::wstring temp_file_path = PathService().appdata_path() + L"\\temp";
	FileHelper().delete_dir(temp_file_path);
	return 0;
}

void Application::init_log()
{
	google::InitGoogleLogging("craftbox");

#ifdef _DEBUG
	google::SetStderrLogging(google::GLOG_WARNING);
#endif

	std::wstring log_dir = PathService().appdata_path();
	log_dir.append(L"\\log");
	::SHCreateDirectory(0, log_dir.c_str());
	char temp[1024] = { 0 };
	WideCharToMultiByte(CP_ACP, 0, log_dir.c_str(), -1, temp, 1024, NULL, NULL);
	static std::string log_dir_str = temp;
	FLAGS_log_dir = log_dir_str;

	google::SetLogDestination(google::GLOG_WARNING, "");
	google::SetLogDestination(google::GLOG_ERROR, "");
	google::SetLogDestination(google::GLOG_FATAL, "");
}

bool Application::handle_singleton_process()
{
	HANDLE exist_exiting_process_handle = ::OpenFileMappingW(FILE_MAP_READ, FALSE, L"zuimc_launcher_exiting_map");
	if (exist_exiting_process_handle)
	{
		LPVOID ptr = ::MapViewOfFile(exist_exiting_process_handle, FILE_MAP_READ, 0, 0, 0);
		if (ptr)
		{
			DWORD pid = *(DWORD*)ptr;
			HANDLE process = ::OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, pid);
			if (process)
			{
				::TerminateProcess(process, 1);
				::WaitForSingleObject(process, 5 * 1000);
				::CloseHandle(process);
			}
			::UnmapViewOfFile(ptr);
		}
		::CloseHandle(exist_exiting_process_handle);
		exist_exiting_process_handle = nullptr;
	}

	singleton_process_runnging_lock_ = ::CreateMutexW(nullptr, TRUE, L"zuimc_launcher_running");
	auto err = ::GetLastError();
	if (err == ERROR_ALREADY_EXISTS || err == ERROR_ACCESS_DENIED)
	{
		HWND main_win = ::FindWindowW(L"zuimc_mainwnd", mcfe::g_app_name);
		if (main_win)
		{
			::ShowWindow(main_win, SW_RESTORE);
			::ShowWindow(main_win, SW_SHOW);
			::SetForegroundWindow(main_win);
			::InvalidateRect(main_win, nullptr, FALSE);
			COPYDATASTRUCT cp;
			cp.dwData = MSG_PASS_APP_PARAM;
			auto cmd = GetCommandLineW();
			cp.lpData = cmd;
			cp.cbData = (wcslen(cmd) + 1) * sizeof(wchar_t);
			::SendMessageW(main_win, WM_COPYDATA, 0, (LPARAM)&cp);
		}
		return false;
	}
	else if (singleton_process_runnging_lock_ == nullptr)
	{
		::MessageBox(0, 0, 0, 0);
	}

	return true;
}

void Application::signal_exiting()
{
	singleton_process_exiting_share_memory_ = ::CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(DWORD), L"zuimc_launcher_exiting_map");
	LPVOID ptr = ::MapViewOfFile(singleton_process_exiting_share_memory_, FILE_MAP_WRITE, 0, 0, 0);
	if (ptr)
	{
		*(DWORD*)ptr = ::GetCurrentProcessId();
		::UnmapViewOfFile(ptr);
	}
}

void Application::EnableIEFeatures()
{
	auto ie_ver = SystemInfo().ie_version();
	if (ie_ver.empty() || ie_ver.length() < 2)
		return;
	if (ie_ver.at(1) == L'.')
		ie_ver = ie_ver.substr(0, 1);
	else
		ie_ver = ie_ver.substr(0, 2);

	CRegKey value_key;
	REGSAM opt = KEY_WRITE;
	if (SystemInfo().is_x64_system())
		opt |= KEY_WOW64_64KEY;

	value_key.Create(HKEY_CURRENT_USER, L"Software\\Microsoft\\Internet Explorer\\Main\\FeatureControl\\FEATURE_BROWSER_EMULATION");
	value_key.Close();

	if (ERROR_SUCCESS == value_key.Open(HKEY_CURRENT_USER, L"Software\\Microsoft\\Internet Explorer\\Main\\FeatureControl\\FEATURE_BROWSER_EMULATION", opt))
	{
		wchar_t buf[1024] = { 0 };
		GetModuleFileNameW(nullptr, buf, 1024);
		auto ptr = PathFindFileNameW(buf);
		if (ptr != nullptr)
		{
			if (ie_ver.compare(L"8") == 0)
			{
				value_key.SetDWORDValue(ptr, 8888);
			}
			else if (ie_ver.compare(L"9") == 0)
			{
				value_key.SetDWORDValue(ptr, 9000);
			}
			else if (ie_ver.compare(L"10") == 0)
			{
				value_key.SetDWORDValue(ptr, 10000);
			}
			else if (ie_ver.compare(L"11") == 0)
			{
				value_key.SetDWORDValue(ptr, 10001);
			}
		}
	}
}

void Application::OnDownloadUpdateSuccess(std::wstring update_exe_path, std::wstring app_name, std::wstring version, std::wstring note)
{
	wchar_t exe_path[1024] = { 0 };
	GetModuleFileNameW(nullptr, exe_path, 1024);
	auto exe_name = ::PathFindFileNameW(exe_path);
	if (exe_name == nullptr)
	{
		DCHECK(0);
		return;
	}

	std::wstring out_of_day_file_path(exe_path, exe_name - exe_path);
	out_of_day_file_path.append(L"need_delete_file");
	if (::PathFileExistsW(out_of_day_file_path.c_str()))
		::DeleteFileW(out_of_day_file_path.c_str());

	::MoveFileW(exe_path, out_of_day_file_path.c_str());

	std::wstring copy_to_path(exe_path, exe_name - exe_path);
	copy_to_path = copy_to_path + app_name + version + L".exe";
	if (::CopyFileW(update_exe_path.c_str(), copy_to_path.c_str(), FALSE))
	{
		UpdateSuccessNotifyWnd wnd;
		wnd.set_updated_version(version);
		wnd.set_updated_note(note);
		if (0 != wnd.Create(0, L"创世神启动器升级提示", WS_VISIBLE | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0))
		{
			wnd.CenterWindow();
			wnd.ShowModal();
		}
	}
}

bool Application::RegisterMCProtocol()
{
	bool ret_val = true;
	CRegKey key;
	auto ret = key.Create(HKEY_CURRENT_USER, L"SOFTWARE\\Classes\\mc\\shell\\open\\command");
	if (ret == 0)
	{
		wchar_t path[1024] = { 0 };
		GetModuleFileNameW(nullptr, path, 1024);
		std::wstring val = L"\"";
		val += path;
		val += L"\" %1";
		key.SetStringValue(NULL, val.c_str(), REG_SZ);

		key.Close();
	}
	else
	{
		ret_val = false;
		LOG(ERROR) << "create reg key formc protocol failed with err " << ret;
	}
	
	ret = key.Open(HKEY_CURRENT_USER, L"SOFTWARE\\Classes\\mc", KEY_WRITE | KEY_WOW64_64KEY);
	if (0 == ret)
	{
		auto ret1 = key.SetStringValue(NULL, L"URL:mc Protocol", REG_SZ);
		auto ret2 = key.SetStringValue(L"URL Protocol", L"", REG_SZ);
		if (ret1 != 0 || ret2 != 0)
		{
			ret_val = false;
			LOG(ERROR) << "set reg key value for mc protocol failed with err " << ret1 << " " << ret2;
		}
	}
	else
	{
		ret_val = false;
		LOG(ERROR) << "open reg key formc protocol failed with err " << ret;
	}

	return ret_val;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPWSTR lpCmdLine, int nCmdShow)
{
	Application app(hInstance);
	return app.run();
}
