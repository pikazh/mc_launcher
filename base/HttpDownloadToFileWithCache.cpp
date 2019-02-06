#include "HttpDownloadToFileWithCache.h"
#include "DownloadCacheManager.h"
#include "glog/logging.h"
#include "asynchronous_task_dispatch.h"
#include "at_exit_manager.h"
#include "NetworkTaskDispatcher.h"

#include <io.h>
#include <shlobj.h>
#include <shlwapi.h>

HttpDownloadToFileWithCache::HttpDownloadToFileWithCache()
{
}

HttpDownloadToFileWithCache::~HttpDownloadToFileWithCache()
{
}

void HttpDownloadToFileWithCache::start_download()
{
	if (state() != kStop)
		return;

	set_cancel(false);
	NetworkTaskDispatcher::global()->add_task(this->shared_from_this());
}

void HttpDownloadToFileWithCache::stop_download()
{
	if (state() == kStop)
		return;

	set_cancel(true);
}

bool HttpDownloadToFileWithCache::is_downloading()
{
	return (state() != kStop);
}

void HttpDownloadToFileWithCache::set_url(const std::string &url)
{
	url_ = url;
}

void HttpDownloadToFileWithCache::set_download_file_path(const std::wstring &path)
{
	save_file_path_ = path;
}

bool HttpDownloadToFileWithCache::on_before_schedule()
{
	download_success_ = false;

	std::wstring dir = save_file_path_;
	*PathFindFileNameW((LPWSTR)dir.data()) = 0;
	::SHCreateDirectory(0, dir.c_str());

	save_file_ = _wfopen(save_file_path_.c_str(), L"wb+");
	if (save_file_ == nullptr)
		return false;

	easy().add<CURLOPT_URL>(url_.c_str());
	easy().add<CURLOPT_WRITEDATA>(save_file_);
	easy().add<CURLOPT_WRITEFUNCTION>((size_t(__cdecl *)(void *, size_t, size_t, void *))::fwrite);

	__super::on_before_schedule();
	return true;
}

bool HttpDownloadToFileWithCache::on_after_schedule()
{
	__super::on_after_schedule();

	AtExitManager at_exit;
	at_exit.register_callback(make_closure([this]()
	{
		fclose(save_file_);
		save_file_ = nullptr;
	}));

	if (!this->is_cancelled() && curl_result() == CURLE_OK)
	{
		curl_easy_getinfo(easy().get_curl(), CURLINFO_RESPONSE_CODE, &http_res_code_);
		if (http_res_code_ == 304)
		{
			auto entry = DownloadCacheManager::GetInstance()->cache_entry_for_url(url_);

			fflush(save_file_);
			chsize(fileno(save_file_), 0);
			fseek(save_file_, 0, SEEK_SET);
			if (!DownloadCacheManager::GetInstance()->read_cache_file(entry, save_file_))
			{
				DCHECK(0);
			}

			download_success_ = true;
		}
		else if (http_res_code_ == 200)
		{
			std::string last_modified_value = last_modified();
			if (!last_modified_value.empty())
			{
				auto entry = DownloadCacheManager::GetInstance()->cache_entry_for_url(url_);
				if (!DownloadCacheManager::GetInstance()->update_cache_file(entry, save_file_, last_modified_value))
					DCHECK(0);
			}

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
		if (curl_result() != CURLE_OK)
		{
			LOG(ERROR) << "download " << url_ << "failed with curl result " << curl_result();
		}

		return false;
	}
		
}

struct curl_slist * HttpDownloadToFileWithCache::on_set_header(struct curl_slist* header)
{
	auto entry = DownloadCacheManager::GetInstance()->cache_entry_for_url(url_);
	auto if_modified_since = DownloadCacheManager::GetInstance()->get_if_modified_since(entry);
	if (!if_modified_since.empty())
	{
		if_modified_since = "If-Modified-Since: " + if_modified_since;
		header = curl_slist_append(header, if_modified_since.c_str());
	}
	header = __super::on_set_header(header);
	return header;
}
