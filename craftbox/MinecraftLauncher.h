#pragma once

#include "base/sigslot.h"
#include "base/asynchronous_task_dispatch.h"
#include "MinecraftProcessInfo.h"

#include <memory>
#include <string>
#include <vector>
#include <thread>

class MinecraftProfile;
class MinecraftUserIdentity;

class MinecraftLauncher
	: public std::enable_shared_from_this<MinecraftLauncher>
{
public:
	MinecraftLauncher(std::shared_ptr<MinecraftProfile> inst, std::shared_ptr<MinecraftUserIdentity> user_id, const std::wstring &java_path, const std::wstring &java_param, const std::vector<std::pair<std::wstring, std::wstring>> extean_mc_params);
	MinecraftLauncher(std::shared_ptr<MinecraftProfile> inst, std::shared_ptr<MinecraftUserIdentity> user_id, const std::wstring &java_path, const std::wstring &java_param);
	virtual ~MinecraftLauncher();

	enum LauncherErrorCode
	{
		kNoError = 0,
		kErrorFileLoss,
		kErrorCannotCreateProcess,
	};
public:
	bool launch();
	sigslot::signal0<> launch_processed;
	bool launch_success() { return success_; }
	LauncherErrorCode error() { return error_code_; }
	std::shared_ptr<MinecraftProcessInfo> proc_info() { return proc_info_; }
	void wait_thread();
protected:
	bool extrace_native_libs(std::wstring extract_path);
	void replace_argument(std::wstring &argument);
	void handle_external_argument(std::wstring &argument);
	void emit_launch_processed_signal(const std::weak_ptr<MinecraftLauncher> &weak_obj, bool success, LauncherErrorCode code);
	void check_and_copy_files();
	std::wstring static extract_craftboxhelper_dll();
private:
	std::shared_ptr<MinecraftProfile> mc_inst_;
	std::shared_ptr<MinecraftUserIdentity> user_id_;
	std::wstring java_path_;
	std::wstring java_param_;
	std::thread launcher_thread_;
	std::vector<std::pair<std::wstring, std::wstring>> extra_minecraft_param_;
	bool success_ = false;
	LauncherErrorCode error_code_ = kNoError;
	std::shared_ptr<AsyncTaskDispatch> async_dispatch_in_create_thread_;
	std::shared_ptr<MinecraftProcessInfo> proc_info_;
};

