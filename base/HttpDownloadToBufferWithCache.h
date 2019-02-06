#pragma once

#include "NetworkTask.h"

class HttpDownloadToBufferWithCache
	: public NetworkTask
{
public:
	HttpDownloadToBufferWithCache();
	virtual ~HttpDownloadToBufferWithCache();

	void start_download();
	void stop_download();
	bool is_downloading();
public:
	bool download_success() { return download_success_; }
	void set_url(const std::string &url);
	std::string url() { return url_; }
	void set_cookie(const std::string &cookie) { cookie_ = cookie; }
	std::string download_buffer() { return buffer_; }
protected:
	virtual bool on_before_schedule() override;
	virtual bool on_after_schedule() override;
	virtual struct curl_slist* on_set_header(struct curl_slist* h);
	static size_t read_function(void *buf, size_t unit_size, size_t count, void* user_data);

private:
	bool download_success_ = false;
	std::string buffer_;
	std::wstring save_file_path_;
	std::string url_;
	long http_res_code_ = 0;
	std::string cookie_;
};

