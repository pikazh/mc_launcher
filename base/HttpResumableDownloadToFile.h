#pragma once

#include "NetworkTask.h"

class HttpResumableDownloadToFile
	: public NetworkTask
{
public:
	HttpResumableDownloadToFile();
	virtual ~HttpResumableDownloadToFile();

	void start_download();
	void stop_download();
	bool is_downloading();

	virtual uint64_t dl_current() override { return total_>0?progress_+ data_buffer_len_ :__super::dl_current(); }
	virtual uint64_t dl_total() override { return total_>0 ? total_ : __super::dl_total(); }

public:
	bool download_success() { return download_success_; }
	void set_url(const std::string &url);
	std::string url() { return url_; }
	void set_download_file_path(const std::wstring &path);
	std::wstring download_file_path() { return save_file_path_; }
protected:
	virtual bool on_before_schedule() override;
	virtual bool on_after_schedule() override;
	virtual struct curl_slist* on_set_header(struct curl_slist* h) override;

	static size_t data_read_function(void* buf, size_t size, size_t count, void* user_data);
private:
	bool download_success_ = false;
	FILE* save_file_ = nullptr;
	uint64_t progress_ = 0;
	uint64_t total_ = 0;
	bool first_write_ = true;
	std::string data_buffer_;
	uint32_t data_buffer_len_ = 0;
	std::wstring save_file_path_;
	std::string url_;
	long http_res_code_ = 0;
};

