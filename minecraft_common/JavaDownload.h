#pragma once

#include "base/NetworkTask.h"
#include "base/sigslot.h"
#include "document.h"

#include <list>
#include <string>

class JavaDownload
	: public NetworkTask
	, public sigslot::has_slots<>
{
public:
	JavaDownload();
	virtual ~JavaDownload();

	enum DL_STEP
	{
		DL_NOT_START = 0,
		DL_URL,
		DL_CONTENT,
		DL_UNCOMPRESS,
	};

public:
	void start_download(uint8_t java_version);
	void stop_download();

	void set_download_dir(const std::wstring &path) { download_java_dir_ = path; }
	std::wstring &download_dir() { return download_java_dir_; }

	DL_STEP download_step() { return step_; }

	sigslot::signal0<> download_java_failed;
	sigslot::signal0<> download_java_success;

protected:
	static size_t java_url_buffer_func(void *ptr, size_t size, size_t nmemb, void *userdata);
	static size_t save_java_zip_func(void *ptr, size_t size, size_t nmemb, void *userdata);

	bool on_before_schedule() override;
	bool on_after_schedule() override;

	void on_network_task_state_changed(std::shared_ptr<NetworkTask>, State state);

	void download_java_zip();

	void on_download_java_failed();
	void on_download_java_success();

	bool verify_download_file();

private:

	char* java_url_buffer_ = nullptr;
	uint32_t java_buffer_size_ = 0;
	uint32_t buffer_content_len = 0;

	rapidjson::Document document_;
	struct JavaDLItem
	{
		std::string url;
		std::string sha1;
	};
	std::list<std::shared_ptr<JavaDLItem>> java_download_items_;

	DL_STEP step_ = DL_NOT_START;

	std::wstring download_java_dir_;
	FILE* file_ptr_ = nullptr;
	std::shared_ptr<JavaDLItem> current_dl_item_;
	std::wstring java_zip_path_;
	bool download_success_ = false;
};

