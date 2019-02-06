#include "NetworkTask.h"
#include "system_info.h"
#include "file_version.h"

#include <codecvt>

NetworkTask::NetworkTask()
{
	easy().add<CURLOPT_FOLLOWLOCATION>(1);
	easy().add<CURLOPT_NOPROGRESS>(0);
	easy().add<CURLOPT_PROGRESSFUNCTION>(NetworkTask::progress_callback);
	easy().add<CURLOPT_PROGRESSDATA>(this);
	easy().add<CURLOPT_CONNECTTIMEOUT>(10);
	easy().add<CURLOPT_HEADERDATA>(this);
	easy().add<CURLOPT_HEADERFUNCTION>(&NetworkTask::header_callback);
	//easy().add<CURLOPT_SSL_VERIFYHOST>(0);
	//easy().add<CURLOPT_SSL_VERIFYSTATUS>(0);
	//easy().add<CURLOPT_SSL_VERIFYPEER>(0);
}

int NetworkTask::progress_callback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
	NetworkTask* task = static_cast<NetworkTask*>(clientp);
	if (task)
	{
		task->dl_curr_ = dlnow;
		task->dl_total_ = dltotal;
	}

	return 0;
}

NetworkTask::~NetworkTask()
{
	if (headers_)
	{
		curl_slist_free_all(headers_);
		headers_ = nullptr;
	}
}

uint32_t NetworkTask::generate_task_id()
{
	static volatile uint32_t id_seed_ = 1;
	return ::InterlockedIncrement(&id_seed_);
}

size_t NetworkTask::header_callback(void *buffer, size_t size, size_t nitems, void *userdata)
{
	auto sz = size*nitems;
	NetworkTask *pthis = (NetworkTask*)(userdata);
	if (pthis)
	{
		pthis->response_header_.append((char*)buffer, sz);
	}

	return sz;
}

bool NetworkTask::on_before_schedule()
{
	response_header_.clear();
	dl_total_ = dl_curr_ = 0;

	if (headers_)
	{
		curl_slist_free_all(headers_);
		headers_ = nullptr;
	}

	headers_ = on_set_header(headers_);
	easy().add<CURLOPT_HTTPHEADER>(headers_);
	return true;
}

bool NetworkTask::on_after_schedule()
{
	if (headers_)
	{
		curl_slist_free_all(headers_);
		headers_ = nullptr;
	}
	return true;
}

std::string NetworkTask::last_modified()
{
	auto begin = response_header_.find("Last-Modified: ");
	if (begin == std::string::npos)
		return "";
	else
	{
		begin += strlen("Last-Modified: ");
		auto end = response_header_.find("\r\n", begin);
		if (end == std::string::npos)
		{
			return response_header_.substr(begin);
		}
		else
		{
			return response_header_.substr(begin, end - begin);
		}
	}
}

struct curl_slist * NetworkTask::on_set_header(struct curl_slist* h)
{
	std::string user_agent;
	{
		user_agent = "User-Agent: CraftBox " + FileVersion::app_file_version() + "; ";
		SystemInfo info;
		user_agent += info.windows_version_string(info.windows_version());
		user_agent += " ";
		user_agent += info.is_x64_system() ? "x64; " : "x86; ";
		user_agent += "IE ";
		std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
		user_agent += converter.to_bytes(info.ie_version());
		user_agent += "; mem ";
		user_agent += std::to_string(info.total_physical_memory_size() >> 20);
	}

	h = curl_slist_append(h, user_agent.c_str());
	return h;
}
