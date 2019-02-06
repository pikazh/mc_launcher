#include "HttpResumableDownloadToFile.h"
#include "glog/logging.h"
#include "asynchronous_task_dispatch.h"
#include "at_exit_manager.h"
#include "NetworkTaskDispatcher.h"

#include <io.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <atlstr.h>
#include <atlconv.h>

HttpResumableDownloadToFile::HttpResumableDownloadToFile()
{
}

HttpResumableDownloadToFile::~HttpResumableDownloadToFile()
{
}

void HttpResumableDownloadToFile::start_download()
{
	if (state() != kStop)
		return;

	set_cancel(false);
	NetworkTaskDispatcher::global()->add_task(this->shared_from_this());
}

void HttpResumableDownloadToFile::stop_download()
{
	if (state() == kStop)
		return;

	set_cancel(true);
}

bool HttpResumableDownloadToFile::is_downloading()
{
	return (state() != kStop);
}

void HttpResumableDownloadToFile::set_url(const std::string &url)
{
	url_ = url;
}

void HttpResumableDownloadToFile::set_download_file_path(const std::wstring &path)
{
	save_file_path_ = path;
}

bool HttpResumableDownloadToFile::on_before_schedule()
{
	download_success_ = false;

	std::wstring dir = save_file_path_;
	*PathFindFileNameW((LPWSTR)dir.data()) = 0;
	::SHCreateDirectory(0, dir.c_str());

	progress_ = 0;
	total_ = 0;
	first_write_ = true;
	data_buffer_len_ = 0;
	data_buffer_.clear();
	std::ifstream progress_file(save_file_path_ + L".dl", std::ios_base::in | std::ios_base::binary);
	if (progress_file.is_open())
	{
		char buff[128] = { 0 };
		progress_file.read(buff, 128);
		std::stringstream ss(buff);
		ss >> progress_;
		ss >> total_;

		if (progress_ == 0)
			total_ = 0;
		else if (progress_ >= total_)
		{
			progress_ = 0;
			total_ = 0;
		}

		progress_file.close();
	}

	if(total_ == 0)
		save_file_ = _wfopen(save_file_path_.c_str(), L"wb+");
	else
	{
		save_file_ = _wfopen(save_file_path_.c_str(), L"rb+");
		if (save_file_ == nullptr)
		{
			progress_ = 0;
			total_ = 0;
			save_file_ = _wfopen(save_file_path_.c_str(), L"wb+");
		}
	}
	
	if (save_file_ == nullptr)
	{
		return false;
	}

	easy().add<CURLOPT_URL>(url_.c_str());
	easy().add<CURLOPT_WRITEDATA>(this);
	easy().add<CURLOPT_WRITEFUNCTION>(data_read_function);

	__super::on_before_schedule();
	return true;
}

bool HttpResumableDownloadToFile::on_after_schedule()
{
	__super::on_after_schedule();

	if (data_buffer_len_ > 0)
	{
		if (first_write_)
		{
			first_write_ = false;
			if (progress_ != 0)
			{
				fflush(save_file_);
				int ret = fseek(save_file_, progress_, SEEK_SET);
				DCHECK(ret == 0);
			}
		}

		fwrite(data_buffer_.c_str(), 1, data_buffer_len_, save_file_);
		data_buffer_len_ = 0;
		data_buffer_.clear();
	}

	AtExitManager at_exit;
	at_exit.register_callback(make_closure([this]()
	{
		fclose(save_file_);
		save_file_ = nullptr;
	}));

	if (!this->is_cancelled() && (curl_result() == CURLE_OK || curl_result() == CURLE_PARTIAL_FILE))
	{
		curl_easy_getinfo(easy().get_curl(), CURLINFO_RESPONSE_CODE, &http_res_code_);
		if (http_res_code_ == 200 || http_res_code_ == 206)
		{
			download_success_ = true;
		}
		else
		{
			LOG(ERROR) << "download " << url_ << "failed with http code " << http_res_code_;
		}

		return true;
	}
	else
	{
		if (curl_result() != CURLE_OK && CURLE_PARTIAL_FILE != curl_result())
		{
			LOG(ERROR) << "download " << url_ << "failed with curl result " << curl_result();
		}

		return false;
	}
}

struct curl_slist* HttpResumableDownloadToFile::on_set_header(struct curl_slist* h)
{
	if (total_ != 0)
	{
		std::string range = "Range: bytes=" + std::to_string(progress_) + "-";
		h = curl_slist_append(h, range.c_str());
	}
	h = __super::on_set_header(h);
	return h;
}

size_t HttpResumableDownloadToFile::data_read_function(void* buf, size_t size, size_t count, void* user_data)
{
	HttpResumableDownloadToFile *pThis = (HttpResumableDownloadToFile*)user_data;
	size_t sz = size*count;
	if (pThis)
	{
		pThis->data_buffer_.append((char*)buf, sz);
		pThis->data_buffer_len_ += sz;
		if (pThis->data_buffer_len_ >= 1024 * 1024)
		{
			if (pThis->first_write_)
			{
				pThis->first_write_ = false;

				if (pThis->progress_ != 0)
				{
					fflush(pThis->save_file_);
					int ret = fseek(pThis->save_file_, pThis->progress_, SEEK_SET);
					DCHECK(ret == 0);
				}
			}

			pThis->progress_ += pThis->data_buffer_len_;
			fwrite(pThis->data_buffer_.c_str(), 1, pThis->data_buffer_len_, pThis->save_file_);
			pThis->data_buffer_len_ = 0;
			pThis->data_buffer_.clear();

			{
				std::ofstream progress_file(pThis->save_file_path_ + L".dl", std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
				if (progress_file.is_open())
				{
					if (pThis->total_ == 0)
					{
						double content_len = 0;
						curl_easy_getinfo(pThis->easy().get_curl(), CURLINFO_CONTENT_LENGTH_DOWNLOAD, &content_len);
						pThis->total_ = content_len;
					}

					std::string progress_info = std::to_string(pThis->progress_);
					progress_info += " ";
					progress_info += std::to_string(pThis->total_);
					progress_file.write(progress_info.c_str(), progress_info.length());

					progress_file.close();
				}
			}
		}
		
	}

	return sz;
}
