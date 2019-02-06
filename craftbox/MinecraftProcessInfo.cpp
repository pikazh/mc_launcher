#include "MinecraftProcessInfo.h"
#include "glog\logging.h"
#include "base\asynchronous_task_dispatch.h"

MinecraftProcessInfoManager* MinecraftProcessInfoManager::global()
{
	static std::shared_ptr<MinecraftProcessInfoManager> inst = std::make_shared<MinecraftProcessInfoManager>();
	return inst.get();
}

void MinecraftProcessInfoManager::add_process_info(std::shared_ptr<MinecraftProcessInfo> info)
{
	DCHECK(!!info && info->proc_id_ != 0 && info->proc_handle_ != nullptr);

	std::lock_guard<std::recursive_mutex> l(lock_);

	auto it = this->infos_.find(info->proc_id_);
	if (it == infos_.end())
	{
		infos_[info->proc_id_] = info;
		HANDLE proc_handle = info->proc_handle_;
		std::weak_ptr<MinecraftProcessInfoManager> weak_obj = this->shared_from_this();
		std::thread t([weak_obj, proc_handle, info]()
		{
			if (WAIT_OBJECT_0 != WaitForSingleObject(proc_handle, INFINITE))
			{
				DCHECK(0);
			}

			DWORD exit_code = 0;
			if (GetExitCodeProcess(proc_handle, &exit_code) && exit_code != 0)
			{
				AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj, info]()
				{
					auto obj = weak_obj.lock();
					if (!obj)
						return;
					obj->process_crash(info);
				}), true);
			}
			else if (GetTickCount() - info->create_time <= 6 * 1000)
			{
				AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj, info]()
				{
					auto obj = weak_obj.lock();
					if (!obj)
						return;
					obj->process_may_crash(info);
				}), true);
			}
			
			AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj, info]()
			{
				auto obj = weak_obj.lock();
				if (!obj)
					return;
				obj->remove_process_info(info);
			}), true);

		});
		t.detach();
	}
	else
	{
		DCHECK(0);
	}
}

void MinecraftProcessInfoManager::remove_process_info(std::shared_ptr<MinecraftProcessInfo> info)
{
	DCHECK(!!info && info->proc_id_ != 0);
	std::lock_guard<std::recursive_mutex> l(lock_);

	auto it = infos_.find(info->proc_id_);
	if (it != infos_.end())
		infos_.erase(it);
	else
	{
		DCHECK(0);
	}
}
