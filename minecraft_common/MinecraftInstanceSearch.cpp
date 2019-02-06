#include "MinecraftInstanceSearch.h"
#include "MinecraftInstanceManager.h"
#include "base\StringHelper.h"
#include "base\asynchronous_task_dispatch.h"
#include "base\PathService.h"

#include <windows.h>
#include <memory>
#include <shlwapi.h>

#define MAX_SERACH_DEPTH	(6)

MinecraftInstanceSearch::MinecraftInstanceSearch()
{
}


MinecraftInstanceSearch::~MinecraftInstanceSearch()
{
}

bool MinecraftInstanceSearch::start_search(bool search_exe_dir_only)
{
	if (searching_)
		return true;

	if (thread_.joinable())
		thread_.join();

	search_exe_dir_only_ = search_exe_dir_only;
	searching_ = true;
	stop_ = false;
	std::thread t([this]()
	{
		this->search_proc();

		std::weak_ptr<MinecraftInstanceSearch> weak_obj = this->shared_from_this();
		AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj]()
		{
			auto obj = weak_obj.lock();
			if (!!obj)
			{
				obj->searching_ = false;
				if (!obj->stop_)
					obj->search_finished(obj->search_exe_dir_only_);
				else
					obj->search_canceled(obj->search_exe_dir_only_);
			}
		}), true);
	});

	thread_ = std::move(t);

	search_started();

	return true;
}

void MinecraftInstanceSearch::stop_search()
{
	stop_ = true;
	if (thread_.joinable())
		thread_.join();
}

void MinecraftInstanceSearch::search_proc()
{
	std::list<std::shared_ptr<SearchItem>> search_paths;

	auto minecraft_inst_manager = MinecraftInstanceManager::GetInstance();
	
	minecraft_inst_manager->check_and_add_minecraft_instance(PathService().exe_dir()+L"\\.minecraft");

	if (search_exe_dir_only_)
		return;

	wchar_t mojang_mc_path[1024] = { 0 };
	ExpandEnvironmentStringsW(L"%appdata%\\.minecraft", mojang_mc_path, 1024);
	minecraft_inst_manager->check_and_add_minecraft_instance(mojang_mc_path);
	auto len = ::GetLogicalDriveStringsW(0, nullptr);
	wchar_t *buf = new wchar_t[len + 2];
	::memset(buf, 0, sizeof(buf));
	std::unique_ptr<wchar_t[]> auto_buf(buf);
	::GetLogicalDriveStringsW(len + 1, buf);
	for (; buf[0] != 0; buf += wcslen(buf)+1)
	{
		UINT uDriverType = DRIVE_UNKNOWN;
		uDriverType = GetDriveTypeW(buf);

		switch (uDriverType)
		{
		case DRIVE_UNKNOWN:
		case DRIVE_NO_ROOT_DIR:
		case DRIVE_CDROM:
			continue;
		}

		if (StrStrIW(buf, L"A:\\") == buf || StrStrIW(buf, L"B:\\") == buf)
		{
			continue;
		}
		std::wstring path = buf;
		wstring_trim_right(path, L'\\');
		search_paths.push_back(std::make_shared<SearchItem>(path, 0));
	}

	while (!search_paths.empty() && !stop_)
	{
		auto search_item = search_paths.front();
		search_paths.pop_front();

		uint8_t depth = search_item->depth;
		std::wstring &path = search_item->path;
		if(depth > MAX_SERACH_DEPTH)
			continue;

		WIN32_FIND_DATAW findFileData;
		HANDLE hFind = FindFirstFile((path+L"\\*.*").c_str(), &findFileData);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				if ((findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
				{
					continue;
				}

				if (findFileData.cFileName[0] == L'.')
				{
					if (wcsicmp(findFileData.cFileName, L".minecraft") == 0)
					{
						MinecraftInstanceManager::GetInstance()->check_and_add_minecraft_instance(path+ L"\\" + findFileData.cFileName);
					}
				}
				else if (findFileData.cFileName[0] == L'$'
					|| wcsicmp(findFileData.cFileName, L"Windows") == 0
					|| wcsicmp(findFileData.cFileName, L"Tencent") == 0
					|| wcsicmp(findFileData.cFileName, L"360") == 0
					|| wcsicmp(findFileData.cFileName, L"baidu") == 0
					|| wcsicmp(findFileData.cFileName, L"VMware") == 0)
				{
					//skip
				}
				else
				{
					search_paths.push_back(std::make_shared<SearchItem>(path+L"\\"+findFileData.cFileName, depth+1));
				}

			} while (FindNextFile(hFind, &findFileData) && !stop_);

			FindClose(hFind);
		}
	}
}
