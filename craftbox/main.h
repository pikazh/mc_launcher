#pragma once

#include "curl_global.h"
#include "base/sigslot.h"

#include <windows.h>

class Application
	: public sigslot::has_slots<>
{
public:
	Application(HINSTANCE app_instance);
	~Application();

public:
	int run();

protected:
	void init_log();
	
	bool handle_singleton_process();
	void signal_exiting();

	void EnableIEFeatures();
	bool RegisterMCProtocol();

	void OnDownloadUpdateSuccess(std::wstring update_exe_path, std::wstring app_name, std::wstring version, std::wstring note);

private:
	HANDLE singleton_process_runnging_lock_ = nullptr;
	HANDLE singleton_process_exiting_share_memory_ = nullptr;
	curl::curl_global curl_global_;
};
