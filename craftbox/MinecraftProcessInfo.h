#pragma once

#include <windows.h>
#include <memory>
#include <map>
#include <mutex>

#include "IPCWindow\IPCTransform.h"
#include "HandleDataCallback.h"
#include "base\sigslot.h"

class MinecraftProcessInfo
{
public:
	MinecraftProcessInfo()
	{

	}

	virtual ~MinecraftProcessInfo()
	{
		if (!!stdout_cb_)
		{
			stdout_cb_->stop();
			stdout_cb_->join();
		}

		if (!!stderr_cb_)
		{
			stderr_cb_->stop();
			stderr_cb_->join();
		}

		if (proc_handle_)
			CloseHandle(proc_handle_);
	}

	DWORD proc_id_ = 0;
	HANDLE proc_handle_ = 0;
	HWND main_wnd_ = 0;
	DWORD create_time = 0;
	std::wstring base_dir_;
	std::wstring version_dir_;
	std::shared_ptr<HandleDataCallback> stdout_cb_;
	std::shared_ptr<HandleDataCallback> stderr_cb_;
	std::shared_ptr<IPCTransform> ipc_trans_;
};

class MinecraftProcessInfoManager
	: public std::enable_shared_from_this<MinecraftProcessInfoManager>
{
public:
	static MinecraftProcessInfoManager* global();

	void add_process_info(std::shared_ptr<MinecraftProcessInfo> info);
	void remove_process_info(std::shared_ptr<MinecraftProcessInfo> info);

	sigslot::signal1<std::shared_ptr<MinecraftProcessInfo>> process_crash;
	sigslot::signal1<std::shared_ptr<MinecraftProcessInfo>> process_may_crash;
private:
	std::map<DWORD, std::shared_ptr<MinecraftProcessInfo>> infos_;
	std::recursive_mutex lock_;
};