#pragma once

#include "DownloadingItemController.h"
#include "base\HttpResumableDownloadToFile.h"

class IntergrationDownloadItemController
	: public DownloadingItemController
{
public:
	IntergrationDownloadItemController(std::shared_ptr<HttpResumableDownloadToFile> task);
	virtual ~IntergrationDownloadItemController();

	virtual void get_download_progress(uint64_t &downloaded, uint64_t &total) override;

	void set_file_size(uint64_t size) { file_size_ = size; }
	void set_file_sha1(const std::string &sha1) { sha1_ = sha1; }
	void set_file_path(const std::wstring &path) { file_save_path_ = path; }
	std::wstring &file_path() { return file_save_path_; }

	virtual void start() override;
	virtual void pause() override;
	virtual void remove() override;


	std::shared_ptr<HttpResumableDownloadToFile> download_obj() { return dl_obj_; }

protected:
	void on_network_task_state_changed(std::shared_ptr<NetworkTask> dl_obj, NetworkTask::State state);
	bool check_download_file_integrity();
private:

	enum CallAction
	{
		kNotCall = -1,
		kStart = 0,
		kPause,
		kRemove,
	};

	std::shared_ptr<HttpResumableDownloadToFile> dl_obj_;
	CallAction call_action_ = kNotCall;

	uint64_t file_size_ = 0;
	std::string sha1_;
	std::wstring file_save_path_;
};

