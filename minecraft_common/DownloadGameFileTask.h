#pragma once

#include "base/NetworkTask.h"

class DownloadGameFileTask
	: public NetworkTask
{

public:
	DownloadGameFileTask();
	virtual ~DownloadGameFileTask();

	enum VerifyMethod
	{
		kNone = 0,
		kSha1,
	};

	enum ErrorCode
	{
		kSuccess = 0,
		kDownloadFailed,
		kVerifyFailed,
		kSaveFileFailed,
		kUserCancelled,
	};

	void set_download_url(const std::string &url);
	
	void set_verify_file_size(uint64_t size) { file_size_ = size; }
	void set_verify_code(VerifyMethod method, const std::string &code)
	{
		verify_method_ = method;
		verify_code_ = code;
	}
	void enable_verify(bool enable) { need_verify_downloaded_file_ = enable; }
	void set_download_file_path(const std::wstring& file_path) { save_file_path_ = file_path; }
	const std::wstring& download_file_path() { return save_file_path_; }

	bool download_success() { return download_success_; }
	ErrorCode download_error() { return error_; }

protected:
	static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata);

	virtual bool on_before_schedule() override;
	virtual bool on_after_schedule() override;
	virtual struct curl_slist* on_set_header(struct curl_slist* h);

	bool verify_download_file();

	bool file_exist_in_writing_set(const std::wstring &path);
	void add_file_path_to_writing_set(const std::wstring &path);
	void remove_file_path_from_writing_set(const std::wstring &path);

private:
	uint64_t file_size_ = 0;
	VerifyMethod verify_method_ = kNone;
	std::string verify_code_;
	std::wstring save_file_path_;
	FILE* save_file_ = nullptr;

	bool download_success_ = false;
	bool need_verify_downloaded_file_ = false;
	ErrorCode error_ = kSuccess;

	static std::set<std::wstring> downloading_file_path;
};

