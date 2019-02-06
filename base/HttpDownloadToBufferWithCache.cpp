#include "HttpDownloadToBufferWithCache.h"
#include "DownloadCacheManager.h"
#include "glog/logging.h"
#include "asynchronous_task_dispatch.h"
#include "at_exit_manager.h"
#include "NetworkTaskDispatcher.h"

#include <io.h>


HttpDownloadToBufferWithCache::HttpDownloadToBufferWithCache()
{
}


HttpDownloadToBufferWithCache::~HttpDownloadToBufferWithCache()
{
}

void HttpDownloadToBufferWithCache::start_download()
{
	if (state() != kStop)
		return;

	set_cancel(false);
	NetworkTaskDispatcher::global()->add_task(this->shared_from_this());
}

void HttpDownloadToBufferWithCache::stop_download()
{
	if (state() == kStop)
		return;

	set_cancel(true);
}

bool HttpDownloadToBufferWithCache::is_downloading()
{
	return (state() != kStop);
}

void HttpDownloadToBufferWithCache::set_url(const std::string &url)
{
	url_ = url;
}

bool HttpDownloadToBufferWithCache::on_before_schedule()
{
	download_success_ = false;

	easy().add<CURLOPT_URL>(url_.c_str());
	easy().add<CURLOPT_WRITEDATA>(this);
	easy().add<CURLOPT_WRITEFUNCTION>(&HttpDownloadToBufferWithCache::read_function);

	if (!cookie_.empty())
	{
		easy().add<CURLOPT_COOKIE>(cookie_.c_str());
	}

	buffer_.clear();
	
	__super::on_before_schedule();
	return true;
}

bool HttpDownloadToBufferWithCache::on_after_schedule()
{
	__super::on_after_schedule();

	if (!this->is_cancelled() && curl_result() == CURLE_OK)
	{
		curl_easy_getinfo(easy().get_curl(), CURLINFO_RESPONSE_CODE, &http_res_code_);
		if (http_res_code_ == 304)
		{
			auto entry = DownloadCacheManager::GetInstance()->cache_entry_for_url(url_);

			buffer_.clear();
			if (!DownloadCacheManager::GetInstance()->read_cache_file(entry, buffer_))
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
				if (!DownloadCacheManager::GetInstance()->update_cache_file(entry, buffer_, last_modified_value))
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

struct curl_slist* HttpDownloadToBufferWithCache::on_set_header(struct curl_slist* h)
{
	auto entry = DownloadCacheManager::GetInstance()->cache_entry_for_url(url_);
	auto if_modified_since = DownloadCacheManager::GetInstance()->get_if_modified_since(entry);
	if (!if_modified_since.empty())
	{
		if_modified_since = "If-Modified-Since: " + if_modified_since;
		h = curl_slist_append(h, if_modified_since.c_str());
	}
	h = __super::on_set_header(h);
	return h;
}

size_t HttpDownloadToBufferWithCache::read_function(void *buf, size_t unit_size, size_t count, void* user_data)
{
	size_t sz = unit_size*count;
	HttpDownloadToBufferWithCache *pThis = (HttpDownloadToBufferWithCache*)(user_data);
	if (pThis)
	{
		pThis->buffer_.append((char*)buf, sz);
	}

	return sz;
}
