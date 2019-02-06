#include "DownloadGameFileTask.h"
#include "glog/logging.h"
#include "base/StringHelper.h"
#include "sha.h"
#include "cryptlib.h"
#include "hex.h"
#include "files.h"
#include "io.h"
#include "base/at_exit_manager.h"
#include "base/DownloadCacheManager.h"

#include <shlwapi.h>
#include <shlobj.h>

std::set<std::wstring> DownloadGameFileTask::downloading_file_path;

size_t DownloadGameFileTask::write_callback(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	FILE* file_ptr = static_cast<FILE*>(userdata);
	if (file_ptr)
	{
		return fwrite(ptr, size, nmemb, file_ptr);
	}
	DCHECK(0);
	return size*nmemb;
}

bool DownloadGameFileTask::on_before_schedule()
{
	if (verify_download_file())
	{
		download_success_ = true;
		return false;
	}

	std::wstring lower_file_path = save_file_path_;
	{
		std::transform(lower_file_path.begin(), lower_file_path.end(), lower_file_path.begin(), towlower);
		if (file_exist_in_writing_set(lower_file_path))
		{
			error_ = kSaveFileFailed;
			download_success_ = false;
			return false;
		}
	}

	std::wstring dir = save_file_path_;
	*PathFindFileNameW((LPWSTR)dir.data()) = 0;
	::SHCreateDirectory(0, dir.c_str());
	save_file_ = _wfopen(save_file_path_.c_str(), L"wb+");
	if (save_file_ == nullptr)
	{
		error_ = kSaveFileFailed;
		download_success_ = false;
		DCHECK(0);
		return false;
	}

	add_file_path_to_writing_set(lower_file_path);

	easy().add<CURLOPT_WRITEFUNCTION>(DownloadGameFileTask::write_callback);
	easy().add<CURLOPT_WRITEDATA>(save_file_);

	__super::on_before_schedule();

	return true;
}

bool DownloadGameFileTask::on_after_schedule()
{
	AtExitManager at_exit;
	__super::on_after_schedule();

	at_exit.register_callback(make_closure([this]() 
	{
		if (save_file_)
		{
			fclose(save_file_);
			save_file_ = nullptr;
		}
	}));

	std::wstring lower_file_path = save_file_path_;
	std::transform(lower_file_path.begin(), lower_file_path.end(), lower_file_path.begin(), towlower);
	remove_file_path_from_writing_set(lower_file_path);

	auto info = easy().get_info<char>(CURLINFO_EFFECTIVE_URL);
	auto &url = info.get().second;

	if (is_cancelled())
	{
		download_success_ = false;
		error_ = ErrorCode::kUserCancelled;
		return true;
	}
	else if (curl_result() != CURLE_OK)
	{
		LOG(ERROR) << " download " << url << " failed with curl_result:" << curl_result();
		download_success_ = false;
		error_ = ErrorCode::kDownloadFailed;
		return true;
	}

	long http_rescode = 0;
	curl_easy_getinfo(easy().get_curl(), CURLINFO_RESPONSE_CODE, &http_rescode);
	if (http_rescode == 304)
	{
		auto entry = DownloadCacheManager::GetInstance()->cache_entry_for_url(url);

		fflush(save_file_);
		chsize(fileno(save_file_), 0);
		fseek(save_file_, 0, SEEK_SET);
		if (!DownloadCacheManager::GetInstance()->read_cache_file(entry, save_file_))
		{
			DCHECK(0);
		}
	}
	else if (http_rescode == 200)
	{
		std::string last_modified_value = last_modified();
		if (!last_modified_value.empty())
		{
			auto entry = DownloadCacheManager::GetInstance()->cache_entry_for_url(url);
			if (!DownloadCacheManager::GetInstance()->update_cache_file(entry, save_file_, last_modified_value))
				DCHECK(0);
		}
	}
	else
	{
		LOG(ERROR) << "download game file with http code:" << http_rescode;
	}


	fclose(save_file_);
	save_file_ = nullptr;

	if (need_verify_downloaded_file_ && !verify_download_file())
	{
		DCHECK(0);
		download_success_ = false;
		error_ = ErrorCode::kVerifyFailed;
	}
	else
	{
		download_success_ = true;
	}
	
	return true;
}

struct curl_slist* DownloadGameFileTask::on_set_header(struct curl_slist* h)
{
	auto info = easy().get_info<char>(CURLINFO_EFFECTIVE_URL);
	auto &url = info.get().second;
	auto entry = DownloadCacheManager::GetInstance()->cache_entry_for_url(url);
	auto if_modified_since = DownloadCacheManager::GetInstance()->get_if_modified_since(entry);
	if (!if_modified_since.empty())
	{
		if_modified_since = "If-Modified-Since: " + if_modified_since;
		h = curl_slist_append(h, if_modified_since.c_str());
	}
	h = __super::on_set_header(h);
	return h;
}

bool DownloadGameFileTask::verify_download_file()
{
#ifdef _DEBUG
	BOOL file_exist = ::PathFileExistsW(save_file_path_.c_str());
#endif
	std::ifstream file_stream(save_file_path_, std::ifstream::binary | std::ifstream::in);
	if (!file_stream.is_open())
	{
#ifdef _DEBUG
		if (file_exist)
			DCHECK(0);
#endif
		return false;
	}
	
	file_stream.seekg(0, std::ios_base::end);
	uint64_t size = file_stream.tellg();
	
	if (size == 0 || (file_size_ != 0 && file_size_ != size))
	{
		return false;
	}

	if (verify_method_ == VerifyMethod::kNone)
		return true;

	file_stream.seekg(0, std::ios_base::beg);

	std::string value;
	if (verify_method_ == VerifyMethod::kSha1)
	{
		CryptoPP::SHA1 hash;
		CryptoPP::FileSource fs(file_stream, true /* PumpAll */,
			new CryptoPP::HashFilter(hash,
				new CryptoPP::HexEncoder(
					new CryptoPP::StringSink(value),
					false
					) // HexEncoder
				) // HashFilter
			); // FileSource
	}
	else
	{
		DCHECK(0);
	}

	if (0 == verify_code_.compare(value))
	{
		return true;
	}

	return false;
}

bool DownloadGameFileTask::file_exist_in_writing_set(const std::wstring & path)
{
	auto it = downloading_file_path.find(path);
	return it != downloading_file_path.end();
}

void DownloadGameFileTask::add_file_path_to_writing_set(const std::wstring & path)
{
	downloading_file_path.insert(path);
}

void DownloadGameFileTask::remove_file_path_from_writing_set(const std::wstring & path)
{
	auto it = downloading_file_path.find(path);
	if (it != downloading_file_path.end())
		downloading_file_path.erase(it);
	else
	{
		DCHECK(0);
	}
}

DownloadGameFileTask::DownloadGameFileTask()
{
	easy().add<CURLOPT_FOLLOWLOCATION>(1L);
}

DownloadGameFileTask::~DownloadGameFileTask()
{
}

void DownloadGameFileTask::set_download_url(const std::string &url)
{
	easy().add<CURLOPT_URL>(url.c_str());
}
