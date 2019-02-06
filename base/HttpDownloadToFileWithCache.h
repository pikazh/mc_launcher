#pragma once

#include "NetworkTask.h"

class HttpDownloadToFileWithCache
	: public NetworkTask
{
public:
	HttpDownloadToFileWithCache();
	virtual ~HttpDownloadToFileWithCache();

	void start_download();
	void stop_download();
	bool is_downloading();
public:
	bool download_success() { return download_success_; }
	void set_url(const std::string &url);
	std::string url() { return url_; }
	void set_download_file_path(const std::wstring &path);
	std::wstring download_file_path() { return save_file_path_; }
protected:
	virtual bool on_before_schedule() override;
	virtual bool on_after_schedule() override;
	virtual struct curl_slist *on_set_header(struct curl_slist* h) override;
private:
	bool download_success_ = false;
	FILE* save_file_ = nullptr;
	std::wstring save_file_path_;
	std::string url_;
	long http_res_code_ = 0;
};

