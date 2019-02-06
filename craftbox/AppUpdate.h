#pragma once

#include "base/NetworkTask.h"
#include "base/NetworkTaskDispatcher.h"

#include <string>

class AppUpdate
	: public NetworkTask
	, public sigslot::has_slots<>
{
public:
	AppUpdate();
	virtual ~AppUpdate();

public:
	sigslot::signal4<std::wstring, std::wstring, std::wstring, std::wstring> update_download_success;

	void begin_update();
	void stop_update();
protected:
	void check_update_exists_and_download();
	virtual bool on_before_schedule() override;
	virtual bool on_after_schedule() override;
	virtual struct curl_slist* on_set_header(struct curl_slist *header) override;

	static size_t on_file_buffer_arrive(void *ptr, size_t size, size_t nmemb, void *userdata);
	static size_t on_update_info_buffer_arrive(void *ptr, size_t size, size_t nmemb, void *userdata);
	
	void on_network_task_state_changed(std::shared_ptr<NetworkTask>, State);
private:
	std::shared_ptr<NetworkTaskDispatcher> network_dispatcher_;
	FILE* update_file_ = nullptr;

	enum Update_State
	{
		kStop = 0,
		kFetchingUpdateInfo,
		kDownloadingUpdateFile,
	};

	Update_State update_state_ = kStop;
	std::string update_info_buffer_;
	std::string url_;
	std::wstring version_;
	std::string sha1_;
	std::wstring app_name_;
	std::wstring note_;
};

