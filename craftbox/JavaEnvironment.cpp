#include "JavaEnvironment.h"
#include "base/PathService.h"
#include "base/system_info.h"
#include "minecraft_common/JavaTest.h"
#include "minecraft_common/JavaFinder.h"
#include "minecraft_common/JavaDownload.h"
#include "base/asynchronous_task_dispatch.h"
#include "AppConfig.h"

#include <windows.h>
#include <shlwapi.h>
#include <atlstr.h>
#include <atlconv.h>
#include <shlobj.h>

std::shared_ptr<JavaEnvironment> JavaEnvironment::GetInstance()
{
	static std::shared_ptr<JavaEnvironment> obj = std::make_shared<JavaEnvironment>();
	return obj;
}

JavaEnvironment::JavaEnvironment()
{
	is_x64_sys_ = SystemInfo().is_x64_system();
	sys_mem_size_in_mb_ = (SystemInfo().total_physical_memory_size() >> 20);
}


JavaEnvironment::~JavaEnvironment()
{
	if (!!java_finder_)
		java_finder_->stop_search();
}

void JavaEnvironment::init()
{
	load_from_profile_file();
}

std::shared_ptr<JavaFinder> JavaEnvironment::search_java()
{
	if (!java_finder_)
	{
		java_finder_ = std::make_shared<JavaFinder>();
	}

	return java_finder_;
}

std::shared_ptr<JavaDownload> JavaEnvironment::download_java()
{
	if (!java_dl_)
	{
		java_dl_ = std::make_shared<JavaDownload>();
	}

	return java_dl_;
}

void JavaEnvironment::load_from_profile_file()
{
	std::wstring xmx = AppConfig::global()->get_value_for_key(L"java_xmx_mb");
	set_java_Xmx_MB(_wtoi(xmx.c_str()));
}

void JavaEnvironment::save_to_profile_file()
{
	AppConfig::global()->set_value_for_key(L"java_xmx_mb", std::to_wstring(java_avail_mem_size_in_mb_));
}

void JavaEnvironment::set_java_Xmx_MB(uint32_t mem_size_in_mb)
{
	java_avail_mem_size_in_mb_ = mem_size_in_mb;
	if (java_avail_mem_size_in_mb_ < 256 || java_avail_mem_size_in_mb_ > sys_mem_size_in_mb_)
	{
		java_avail_mem_size_in_mb_ = default_java_Xmx_MB();
	}
}

bool JavaEnvironment::try_add_java(const std::wstring &path, bool set_default)
{
	bool x64_java = false;
	std::string java_version;
	bool ret = false;
	if (JavaTest().get_java_version(path, java_version, x64_java) && x64_java == is_x64_sys_ && java_version.substr(0, 3).compare("1.6") > 0)
	{
		ret = insert_java_record(path, java_version);
		if (ret)
		{
			if(set_default)
				set_select_java_path(path);

			std::weak_ptr<JavaEnvironment> weak_obj = this->shared_from_this();
			AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj, path, java_version]()
			{
				auto obj = weak_obj.lock();
				if (!!obj)
				{
					obj->java_added(path, java_version);
				}
			}), true);
			
		}
	}

	return ret;
}

bool JavaEnvironment::insert_java_record(const std::wstring &java_path, const std::string &version)
{
	CAtlString str = java_path.c_str();
	std::wstring java_path_lowercast = str.MakeLower();
	bool ret = false;

	std::lock_guard<std::recursive_mutex> l(lock_);

	if (java_path_compare_set_.find(java_path_lowercast) == java_path_compare_set_.end())
	{
		java_path_compare_set_.insert(java_path_lowercast);
		java_paths_and_vers_.insert(std::make_pair(java_path, version));
		ret = true;
	}

	return ret;
}

uint32_t JavaEnvironment::default_java_Xmx_MB()
{
	if (!is_x64_sys_)
	{
		if (sys_mem_size_in_mb_ >= 1024)
			return 1024;
		else if (sys_mem_size_in_mb_ >= 512)
			return 512;
		else
			return sys_mem_size_in_mb_;
	}

	if (sys_mem_size_in_mb_ >= 2048)
		return 2048;
	else if (sys_mem_size_in_mb_ >= 1024)
	{
		return 1024;
	}
	else
		return sys_mem_size_in_mb_;
}

void JavaEnvironment::reset_select_java_path()
{
	std::lock_guard<std::recursive_mutex> l(lock_);
	if (select_java_path_.empty())
	{
		auto it = java_paths_and_vers_.begin();
		if (it != java_paths_and_vers_.end())
		{
			select_java_path_ = it->first;
		}
	}
}

