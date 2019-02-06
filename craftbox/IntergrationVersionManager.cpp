#include "IntergrationVersionManager.h"
#include "base/asynchronous_task_dispatch.h"
#include "glog/logging.h"

#include <memory>

IntergrationVersionManager* IntergrationVersionManager::global()
{
	static std::shared_ptr<IntergrationVersionManager> obj = std::make_shared<IntergrationVersionManager>();
	return obj.get();
}

IntergrationVersionManager::IntergrationVersionManager()
{
}

IntergrationVersionManager::~IntergrationVersionManager()
{
}

std::map<std::string, std::list<std::tuple<std::shared_ptr<HttpResumableDownloadToFile>, std::string, uint64_t, std::wstring, std::wstring>>> IntergrationVersionManager::downloading_instances()
{
	std::lock_guard<decltype(lock_)> l(lock_);

	return downloading_insts_;
}

std::shared_ptr<HttpResumableDownloadToFile> IntergrationVersionManager::download_instance(const std::string &url, const std::wstring &download_path, const std::string &sha1, uint64_t size, std::wstring extrace_path, std::wstring version_str, bool *is_exist_before /*= nullptr*/)
{
	if (is_exist_before)
		*is_exist_before = false;

	std::lock_guard<decltype(lock_)> l(lock_);

	auto it = downloading_insts_.find(url);
	if (it != downloading_insts_.end())
	{
		auto &objs = it->second;

		for (auto it2 = objs.begin(); it2 != objs.end(); ++it2)
		{
			auto dl_path = std::get<3>(*it2);
			if (0 == wcsicmp(dl_path.c_str(), extrace_path.c_str()))
			{
				if (is_exist_before)
					*is_exist_before = true;
				return std::get<0>(*it2);
			}
		}
	}

	auto inst = std::make_shared<HttpResumableDownloadToFile>();
	inst->set_url(url);
	inst->set_download_file_path(download_path);

	auto t = std::make_tuple(inst, sha1, size, extrace_path, version_str);
	downloading_insts_[url].push_front(t);

	std::weak_ptr<IntergrationVersionManager> weak_obj = this->shared_from_this();
	AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj, inst, sha1, size, extrace_path, version_str]()
	{
		auto obj = weak_obj.lock();
		if (!!obj)
			obj->download_inst_added(inst, sha1, size, extrace_path, version_str);
	}), true);

	return inst;
}

void IntergrationVersionManager::remove_download_instance(std::shared_ptr<HttpResumableDownloadToFile> inst)
{
	std::lock_guard<decltype(lock_)> l(lock_);

	auto it = downloading_insts_.find(inst->url());
	if (it != downloading_insts_.end())
	{
		auto &objs = it->second;
		for (auto it2 = objs.begin(); it2 != objs.end(); ++it2)
		{
			auto obj = std::get<0>(*it2);
			if (inst->task_id() == obj->task_id())
			{
				objs.erase(it2);

				DeleteFileW(inst->download_file_path().c_str());
				DeleteFileW(std::wstring(inst->download_file_path() + L".dl").c_str());

				if (objs.empty())
				{
					downloading_insts_.erase(it);
				}

				return;
			}
		}

		DCHECK(0);
	}
	else
	{
		DCHECK(0);
	}
}
