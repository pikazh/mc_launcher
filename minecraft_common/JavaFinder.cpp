#include "JavaFinder.h"
#include "base\SystemHelper.h"
#include "base\asynchronous_task_dispatch.h"
#include "glog\logging.h"
#include "base\at_exit_manager.h"
#include "base\StringHelper.h"
#include "base\PathService.h"
#include "JavaTest.h"
#include "base\system_info.h"
#include "craftbox\JavaEnvironment.h"

#include <atlbase.h>
#include <atlstr.h>
#include <Strsafe.h>
#include <shlobj.h>

#define MAX_SERACH_DEPTH (5)

JavaFinder::JavaFinder()
{
}

JavaFinder::~JavaFinder()
{
	stop_search();

	if (thread_.joinable())
		thread_.join();
}

bool JavaFinder::begin_search()
{
	if (is_searching_)
		return true;
	if (thread_.joinable())
		thread_.join();

	stop_search_ = false;
	is_searching_ = true;
	std::thread t([this]()
	{
		this->search_in_client_dir();
		this->search_in_mojang_dir();
		this->search_in_reg();
		this->search_in_path_env();

		bool no_java_found = false;
		{
			auto javas = JavaEnvironment::GetInstance()->java_paths_and_vers();
			if (javas.empty())
				no_java_found = true;
		}

		if(no_java_found && !stop_search_)
			this->search_in_file_system();

		if (stop_search_)
			this->emit_search_cancelled();
		else
			this->emit_search_finished();

		
	});

	thread_ = std::move(t);

	return true;
}

void JavaFinder::stop_search()
{
	stop_search_ = true;
}

void JavaFinder::search_in_reg()
{
	CRegKey reg;
	if (ERROR_SUCCESS == reg.Open(HKEY_LOCAL_MACHINE, L"SOFTWARE\\JavaSoft\\Java Development Kit", KEY_READ | KEY_WOW64_64KEY))
	{
		long ret = 0;
		DWORD index = 0;
		do
		{
			wchar_t java_version[128] = { 0 };
			DWORD len = _countof(java_version);
			ret = reg.EnumKey(index++, java_version, &len);
			if (ret == ERROR_SUCCESS)
			{
				CRegKey value_key;
				std::wstring key = L"SOFTWARE\\JavaSoft\\Java Development Kit\\";
				key += java_version;
				if (ERROR_SUCCESS == value_key.Open(HKEY_LOCAL_MACHINE, key.c_str(), KEY_READ | KEY_WOW64_64KEY))
				{
					wchar_t java_path[1024] = { 0 };
					DWORD java_path_len = _countof(java_path);
					if (ERROR_SUCCESS == value_key.QueryStringValue(L"JavaHome", java_path, &java_path_len))
					{
						wcscat(java_path, L"\\bin\\Javaw.exe");
						if (PathFileExistsW(java_path) && SystemHelper().VerifyEmbeddedSignature(java_path))
						{
							JavaEnvironment::GetInstance()->try_add_java(java_path);
						}
					}
				}
			}
			else
			{
				break;
			}
		} while (!stop_search_);

		reg.Close();
	}

	if (stop_search_)
		return;

	if (ERROR_SUCCESS == reg.Open(HKEY_LOCAL_MACHINE, L"SOFTWARE\\JavaSoft\\Java Runtime Environment", KEY_READ | KEY_WOW64_64KEY))
	{
		long ret = 0;
		DWORD index = 0;
		do
		{
			wchar_t java_version[128] = { 0 };
			DWORD len = _countof(java_version);
			ret = reg.EnumKey(index++, java_version, &len);
			if (ret == ERROR_SUCCESS)
			{
				CRegKey value_key;
				std::wstring key = L"SOFTWARE\\JavaSoft\\Java Runtime Environment\\";
				key += java_version;
				if (ERROR_SUCCESS == value_key.Open(HKEY_LOCAL_MACHINE, key.c_str(), KEY_READ | KEY_WOW64_64KEY))
				{
					wchar_t java_path[1024] = { 0 };
					DWORD java_path_len = _countof(java_path);
					if (ERROR_SUCCESS == value_key.QueryStringValue(L"JavaHome", java_path, &java_path_len))
					{
						wcscat(java_path, L"\\bin\\Javaw.exe");
						if (PathFileExistsW(java_path) && SystemHelper().VerifyEmbeddedSignature(java_path))
						{
							JavaEnvironment::GetInstance()->try_add_java(java_path);
						}
					}
				}
			}
			else
			{
				break;
			}
		} while (!stop_search_);

		reg.Close();
	}
}

void JavaFinder::emit_search_finished()
{
	std::weak_ptr<JavaFinder> weak_obj = this->shared_from_this();
	AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj]()
	{
		auto obj = weak_obj.lock();
		if (!!obj)
		{
			obj->is_searching_ = false;
			obj->search_finished();
		}
	}), true);
}

void JavaFinder::emit_search_cancelled()
{
	std::weak_ptr<JavaFinder> weak_obj = this->shared_from_this();
	AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj]()
	{
		auto obj = weak_obj.lock();
		if (!!obj)
		{
			obj->is_searching_ = false;
			obj->search_cancelled();
		}
	}), true);
}

void JavaFinder::search_in_mojang_dir()
{
	CRegKey reg;

	if (ERROR_SUCCESS == reg.Open(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Mojang\\Minecraft", KEY_READ))
	{
		long ret = 0;
		DWORD index = 0;

		wchar_t java_path[1024] = { 0 };
		DWORD java_path_len = _countof(java_path);
		if (ERROR_SUCCESS == reg.QueryStringValue(L"InstallDirNew", java_path, &java_path_len))
		{
			std::list<std::wstring> search_dirs;
			std::wstring path = java_path;
			wstring_trim_right(path, L'\\');
			search_dirs.push_back(std::move(path));

			while (!stop_search_ && !search_dirs.empty())
			{
				std::wstring path = search_dirs.front();
				search_dirs.pop_front();
				WIN32_FIND_DATAW findFileData;
				HANDLE hFind = FindFirstFileW((path + L"\\*.*").c_str(), &findFileData);
				if (hFind != INVALID_HANDLE_VALUE)
				{
					do
					{
						if (wcscmp(findFileData.cFileName, L".") == 0 || wcscmp(findFileData.cFileName, L"..") == 0)
							continue;

						if ((findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
						{
							search_dirs.push_back(path + L"\\" + findFileData.cFileName);
							continue;
						}
						else if ((findFileData.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_SPARSE_FILE)) != 0)
						{
							continue;
						}

						if (wcsicmp(findFileData.cFileName, L"Javaw.exe") == 0)
						{
							std::wstring file_path = path + L"\\" + findFileData.cFileName;
							if (SystemHelper().VerifyEmbeddedSignature(file_path.c_str()))
							{
								JavaEnvironment::GetInstance()->try_add_java(file_path);
								break;
							}
						}
					} while (!stop_search_ && ::FindNextFileW(hFind, &findFileData));

					::FindClose(hFind);
				}
			}

		}

		reg.Close();
	}
}

void JavaFinder::search_in_client_dir()
{
	std::wstring app_dir = PathService().exe_dir();
	std::list<std::wstring> search_dirs;
	search_dirs.push_back(std::move(app_dir));

	while (!stop_search_ && !search_dirs.empty())
	{
		std::wstring path = search_dirs.front();
		search_dirs.pop_front();
		WIN32_FIND_DATAW findFileData;
		HANDLE hFind = FindFirstFileW((path + L"\\*.*").c_str(), &findFileData);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (wcscmp(findFileData.cFileName, L".") == 0 || wcscmp(findFileData.cFileName, L"..") == 0)
					continue;

				if ((findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
				{
					search_dirs.push_back(path + L"\\" + findFileData.cFileName);
					continue;
				}
				else if ((findFileData.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_SPARSE_FILE)) != 0)
				{
					continue;
				}

				if (wcsicmp(findFileData.cFileName, L"Javaw.exe") == 0)
				{
					std::wstring file_path = path + L"\\" + findFileData.cFileName;
					if (SystemHelper().VerifyEmbeddedSignature(file_path.c_str()))
					{
						JavaEnvironment::GetInstance()->try_add_java(file_path);
						break;
					}
				}
			} while (!stop_search_ && ::FindNextFileW(hFind, &findFileData));

			::FindClose(hFind);
		}
	}
}

void JavaFinder::search_in_path_env()
{
	uint32_t len = 50 * 1024;
	wchar_t * buf = new wchar_t[len];
	std::unique_ptr<wchar_t[]> auto_buf(buf);

	memset(buf, 0, sizeof(wchar_t)*len);
	if (0 == GetEnvironmentVariableW(L"Path", buf, len))
	{
		DCHECK(0);
		return;
	}

	std::list<std::wstring> search_dirs;
	std::wstring paths = buf;
	auto index = -1;
	do
	{
		auto begin_index = index + 1;
		index = paths.find(L';', begin_index);
		if (index != std::wstring::npos)
		{
			search_dirs.push_back(paths.substr(begin_index, index - begin_index));
		}
		else
		{
			search_dirs.push_back(paths.substr(begin_index));
		}
	} while (index != std::wstring::npos);

	while (!stop_search_ && !search_dirs.empty())
	{
		std::wstring path = search_dirs.front();
		search_dirs.pop_front();
		wstring_trim_right(path, L'\\');
		WIN32_FIND_DATAW findFileData;
		HANDLE hFind = FindFirstFileW((path + L"\\*.*").c_str(), &findFileData);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (wcscmp(findFileData.cFileName, L".") == 0 || wcscmp(findFileData.cFileName, L"..") == 0)
					continue;

				if ((findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
				{
					continue;
				}
				else if ((findFileData.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_SPARSE_FILE)) != 0)
				{
					continue;
				}

				if (wcsicmp(findFileData.cFileName, L"Javaw.exe") == 0)
				{
					std::wstring file_path = path + L"\\" + findFileData.cFileName;
					if (SystemHelper().VerifyEmbeddedSignature(file_path.c_str()))
					{
						JavaEnvironment::GetInstance()->try_add_java(file_path);
						break;
					}
				}
			} while (!stop_search_ && ::FindNextFileW(hFind, &findFileData));

			::FindClose(hFind);
		}
	}

}

void JavaFinder::search_in_file_system()
{
	struct SearchItem
	{
		std::wstring path;
		uint8_t depth = 0;

		SearchItem(const std::wstring &path, uint8_t depth)
		{
			this->path = path;
			this->depth = depth;
		}
	};

	std::list<std::shared_ptr<SearchItem>> search_paths;

	auto len = ::GetLogicalDriveStringsW(0, nullptr);
	wchar_t *buf = new wchar_t[len + 2];
	::memset(buf, 0, sizeof(buf));
	std::unique_ptr<wchar_t[]> auto_buf(buf);
	::GetLogicalDriveStringsW(len + 1, buf);
	for (; buf[0] != 0; buf += wcslen(buf) + 1)
	{
		UINT uDriverType = DRIVE_UNKNOWN;
		uDriverType = GetDriveTypeW(buf);
		if(uDriverType != DRIVE_FIXED)
			continue;

		std::wstring path = buf;
		wstring_trim_right(path, L'\\');
		search_paths.push_back(std::make_shared<SearchItem>(path, 0));
	}

	while (!search_paths.empty() && !stop_search_)
	{
		auto search_item = search_paths.front();
		search_paths.pop_front();

		uint8_t depth = search_item->depth;
		std::wstring &path = search_item->path;
		if (depth > MAX_SERACH_DEPTH)
			continue;

		WIN32_FIND_DATAW findFileData;
		HANDLE hFind = FindFirstFile((path + L"\\*.*").c_str(), &findFileData);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				if ((findFileData.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_SPARSE_FILE)) != 0)
				{
					continue;
				}
				if ((findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
				{
					if (wcscmp(findFileData.cFileName, L".") == 0 || wcscmp(findFileData.cFileName, L"..") == 0)
						continue;
					if (findFileData.cFileName[0] == L'$'
						|| wcsicmp(findFileData.cFileName, L"Windows") == 0
						|| wcsicmp(findFileData.cFileName, L"Tencent") == 0
						|| wcsicmp(findFileData.cFileName, L"360") == 0
						|| wcsicmp(findFileData.cFileName, L"baidu") == 0
						|| wcsicmp(findFileData.cFileName, L"VMware") == 0)
					{
						continue;
					}

					search_paths.push_back(std::make_shared<SearchItem>(path + L"\\" + findFileData.cFileName, depth + 1));
				}
				else
				{
					if (wcsicmp(findFileData.cFileName, L"Javaw.exe") == 0)
					{
						std::wstring file_path = path + L"\\" + findFileData.cFileName;
						if (SystemHelper().VerifyEmbeddedSignature(file_path.c_str()))
						{
							JavaEnvironment::GetInstance()->try_add_java(file_path);
							break;
						}
					}
				}

			} while (FindNextFile(hFind, &findFileData) && !stop_search_);

			FindClose(hFind);
		}
	}
}

