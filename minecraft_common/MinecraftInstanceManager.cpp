#include "MinecraftInstanceManager.h"
#include "MinecraftProfile.h"
#include "base\StringHelper.h"
#include "base\asynchronous_task_dispatch.h"
#include "glog\logging.h"

#include <algorithm>
#include <windows.h>
#include <shlwapi.h>
#include <atlstr.h>

MinecraftInstanceManager* MinecraftInstanceManager::GetInstance()
{
	static std::shared_ptr<MinecraftInstanceManager> inst(new MinecraftInstanceManager);
	return inst.get();
}

MinecraftInstanceManager::MinecraftInstanceManager()
{
}

MinecraftInstanceManager::~MinecraftInstanceManager()
{
}

void MinecraftInstanceManager::check_and_add_minecraft_instance(const std::wstring &base_dir)
{
	std::wstring base_dir_formated = base_dir;
	if (base_dir_formated.length() < 3 || base_dir_formated[1] != ':')
		return;

	wstring_trim_right(base_dir_formated, L'\\');
	
	WIN32_FIND_DATAW findFileData;
	HANDLE hFind = FindFirstFileW((base_dir_formated + L"\\versions\\*.*").c_str(), &findFileData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if((findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
				continue;
			if(wcscmp(findFileData.cFileName, L".") == 0 || wcscmp(findFileData.cFileName, L"..") == 0)
				continue;

			std::wstring profile_json_file = base_dir_formated + L"\\versions\\" + findFileData.cFileName + L"\\" + findFileData.cFileName + L".json";
			if (::PathFileExistsW(profile_json_file.c_str()) && !::PathIsDirectoryW(profile_json_file.c_str()))
			{
				std::shared_ptr<MinecraftInstanceItem> item(new MinecraftInstanceItem);
				item->base_dir = base_dir_formated;
				item->version_str = findFileData.cFileName;
				add_minecraft_instance(base_dir_formated, findFileData.cFileName, item);
			}
		} while (::FindNextFileW(hFind, &findFileData));

		::FindClose(hFind);
	}

}

void MinecraftInstanceManager::check_and_add_minecraft_instance(const std::wstring &base_dir, const std::wstring &version_str, const std::wstring &note)
{
	std::wstring base_dir_formated = base_dir;
	if (base_dir_formated.length() < 3 || base_dir_formated[1] != ':')
		return;

	wstring_trim_right(base_dir_formated, L'\\');

	std::wstring version_str_formated = version_str;
	wstring_trim(version_str_formated, L'\\');
	if (version_str_formated.empty())
		return;

	std::wstring profile_json_file = base_dir_formated + L"\\versions\\" + version_str_formated + L"\\" + version_str_formated + L".json";
	if (::PathFileExistsW(profile_json_file.c_str()) && !::PathIsDirectoryW(profile_json_file.c_str()))
	{
		std::shared_ptr<MinecraftInstanceItem> item(new MinecraftInstanceItem);
		item->base_dir = base_dir_formated;
		item->version_str = version_str_formated;
		item->note = note;
		add_minecraft_instance(base_dir_formated, version_str_formated, item);
	}
}

void MinecraftInstanceManager::remove_minecraft_instance(const std::wstring & base_dir, const std::wstring & version_str)
{
	std::wstring base_dir_formated = base_dir;
	std::wstring version_str_formated = version_str;
	wstring_trim_right(base_dir_formated, L'\\');
	wstring_trim(version_str_formated, L'\\');
	std::wstring base_dir_formmatted = CAtlString(base_dir_formated.c_str()).MakeLower();
	std::wstring version_str_formatted = CAtlString(version_str_formated.c_str()).MakeLower();

	std::lock_guard<std::recursive_mutex> l(lock_);

	auto it = local_insts_.find(base_dir_formmatted);
	if (it != local_insts_.end())
	{
		auto &vers = it->second;
		auto version_it = vers.find(version_str_formatted);
		if (version_it != vers.end())
		{
			auto item = version_it->second;
			if (default_local_inst_.get() == version_it->second.get())
			{
				default_local_inst_.reset();
			}

			vers.erase(version_it);

			std::weak_ptr<MinecraftInstanceManager> weak_obj = this->shared_from_this();
			AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj, item]()
			{
				auto obj = weak_obj.lock();
				if (!!obj)
				{
					obj->instances_removed(item);
				}
			}), true);
		}
		else
		{
			DCHECK(0);
		}

		if (vers.empty())
		{
			local_insts_.erase(it);
		}
	}
	else
	{
		DCHECK(0);
	}
}

std::map<std::wstring, std::map<std::wstring, std::shared_ptr<MinecraftInstanceManager::MinecraftInstanceItem>>> MinecraftInstanceManager::local_instances()
{
	std::lock_guard<std::recursive_mutex> l(lock_);
	return local_insts_;
}

std::map<std::wstring, std::map<std::wstring, std::shared_ptr<MinecraftDownload>>> MinecraftInstanceManager::downloading_instances()
{
	std::lock_guard<std::recursive_mutex> l(lock_);
	return downloading_insts_;

}

void MinecraftInstanceManager::add_minecraft_instance(const std::wstring &base_dir_param, const std::wstring &version_str_param, std::shared_ptr<MinecraftInstanceItem> &item)
{
	std::wstring base_dir_formmatted = CAtlString(base_dir_param.c_str()).MakeLower();
	std::wstring version_str_formatted = CAtlString(version_str_param.c_str()).MakeLower();

	std::lock_guard<std::recursive_mutex> l(lock_);

	auto it = downloading_insts_.find(base_dir_formmatted);
	if (it != downloading_insts_.end())
	{
		auto &vers = it->second;
		auto version_it = vers.find(version_str_formatted);
		if (version_it != vers.end())
		{
			return;
		}
	}

	bool not_exist_before = false;
	auto it_for_set = local_insts_.find(base_dir_formmatted);
	if (it_for_set == local_insts_.end())
	{
		std::map<std::wstring, std::shared_ptr<MinecraftInstanceItem>> list;
		list[version_str_formatted] = item;
		local_insts_.insert(std::make_pair(base_dir_formmatted, std::move(list)));
		not_exist_before = true;
	}
	else
	{
		auto &obj_set = it_for_set->second;
		auto it_for_obj = obj_set.find(version_str_formatted);
		if (it_for_obj == obj_set.end())
		{
			obj_set.insert(std::make_pair(version_str_formatted, item));
			not_exist_before = true;
		}
	}

	if (not_exist_before)
	{
		std::weak_ptr<MinecraftInstanceManager> weak_obj = this->shared_from_this();
		AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj, item]()
		{
			auto obj = weak_obj.lock();
			if (!!obj)
			{
				obj->instances_added(item);
			}
		}), true);
	}
}

std::shared_ptr<MinecraftDownload> MinecraftInstanceManager::download_mc_instance(const std::wstring &base_dir, const std::wstring &version_str, const std::string &profile_url, bool *is_exist_before)
{
	std::wstring base_dir_formmatted = CAtlString(base_dir.c_str()).MakeLower();
	std::wstring version_str_formatted = CAtlString(version_str.c_str()).MakeLower();

	if (is_exist_before)
		*is_exist_before = false;

	std::lock_guard<std::recursive_mutex> l(lock_);

	auto it = downloading_insts_.find(base_dir_formmatted);
	if (it != downloading_insts_.end())
	{
		auto &vers = it->second;
		auto version_it = vers.find(version_str_formatted);
		if (version_it != vers.end())
		{
			if (is_exist_before)
				*is_exist_before = true;
			return version_it->second;
		}
	}
	
	std::shared_ptr<MinecraftDownload> dl_inst(new MinecraftDownload(base_dir, version_str, profile_url));
	downloading_insts_[base_dir_formmatted][version_str_formatted] = dl_inst;

	std::weak_ptr<MinecraftInstanceManager> weak_obj = this->shared_from_this();
	AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj, dl_inst]()
	{
		auto obj = weak_obj.lock();
		if (!!obj)
		{
			obj->download_inst_added(dl_inst);
		}
	}), true);

	return dl_inst;
}

void MinecraftInstanceManager::remove_download_instance(std::shared_ptr<MinecraftDownload> inst)
{
	std::wstring base_dir_formmatted = CAtlString(inst->base_dir().c_str()).MakeLower();
	std::wstring version_str_formatted = CAtlString(inst->version_str().c_str()).MakeLower();

	std::lock_guard<std::recursive_mutex> l(lock_);
	auto it = downloading_insts_.find(base_dir_formmatted);
	if (it != downloading_insts_.end())
	{
		auto &vers = it->second;
		auto version_it = vers.find(version_str_formatted);
		if (version_it != vers.end())
		{
			vers.erase(version_it);
		}
		else
		{
			DCHECK(0);
			LOG(ERROR) << "sth is wrong...";
		}
	}
	else
	{
		DCHECK(0);
		LOG(ERROR) << "sth is wrong...";
	}
}

void MinecraftInstanceManager::set_default_instance(const std::wstring &base_dir, const std::wstring &version_str)
{
	std::wstring base_dir_formated = base_dir;
	std::wstring version_str_formated = version_str;
	wstring_trim_right(base_dir_formated, L'\\');
	wstring_trim(version_str_formated, L'\\');
	std::wstring base_dir_formmatted = CAtlString(base_dir_formated.c_str()).MakeLower();
	std::wstring version_str_formatted = CAtlString(version_str_formated.c_str()).MakeLower();

	std::lock_guard<std::recursive_mutex> l(lock_);

	auto it = local_insts_.find(base_dir_formmatted);
	if (it != local_insts_.end())
	{
		auto &vers = it->second;
		auto version_it = vers.find(version_str_formatted);
		if (version_it != vers.end())
		{
			default_local_inst_ = version_it->second;
		}
	}
}

void MinecraftInstanceManager::modify_minecraft_instance_info(MinecraftInstanceItem &item)
{
	std::wstring base_dir_formated = item.base_dir;
	std::wstring version_str_formated = item.version_str;
	wstring_trim_right(base_dir_formated, L'\\');
	wstring_trim(version_str_formated, L'\\');
	std::wstring base_dir_formmatted = CAtlString(base_dir_formated.c_str()).MakeLower();
	std::wstring version_str_formatted = CAtlString(version_str_formated.c_str()).MakeLower();

	std::lock_guard<std::recursive_mutex> l(lock_);

	auto it = local_insts_.find(base_dir_formmatted);
	if (it != local_insts_.end())
	{
		auto &vers = it->second;
		auto version_it = vers.find(version_str_formatted);
		if (version_it != vers.end())
		{
			version_it->second->note = item.note;
		}
	}
}

void MinecraftInstanceManager::remove_all_instance()
{
	std::lock_guard<std::recursive_mutex> l(lock_);

	local_insts_.clear();
	default_local_inst_.reset();
	
	std::weak_ptr<MinecraftInstanceManager> weak_obj = this->shared_from_this();
	AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj]()
	{
		auto obj = weak_obj.lock();
		if (!!obj)
		{
			obj->instances_all_removed();
		}
	}), true);
}
