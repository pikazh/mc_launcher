#include "AppUpdate.h"
#include "base/PathService.h"
#include "base/JsonHelper.h"
#include "document.h"
#include "base/file_version.h"
#include "sha.h"
#include "cryptlib.h"
#include "hex.h"
#include "files.h"
#include "minecraft_common/MinecraftCommon.h"
#include "base/DownloadCacheManager.h"
#include "base/system_info.h"
#include "base/FileHelper.h"
#include "base/url_encode.h"
#include "base/StringHelper.h"
#include "DataReport/DataReport.h"

#include <shlobj.h>
#include <fstream>
#include <codecvt>
#include <shlwapi.h>

AppUpdate::AppUpdate()
{
	network_dispatcher_ = std::make_shared<NetworkTaskDispatcher>();
	network_dispatcher_->start_schedule();

	this->state_changed.connect(this, &AppUpdate::on_network_task_state_changed);
}

AppUpdate::~AppUpdate()
{
	network_dispatcher_->stop_schedule();
}

void AppUpdate::begin_update()
{
	if (update_state_ != kStop)
		return;

	update_state_ = kFetchingUpdateInfo;

	network_dispatcher_->add_task(this->shared_from_this());
}

void AppUpdate::stop_update()
{
	this->set_cancel(true);
}

void AppUpdate::check_update_exists_and_download()
{
	bool need_download = true;
	std::wstring update_exe_path = PathService().appdata_path() + L"\\update.exe";
	if (::PathIsDirectoryW(update_exe_path.c_str()))
		FileHelper().delete_dir(update_exe_path);
	else
	{
		std::ifstream file_stream(update_exe_path, std::ifstream::binary | std::ifstream::in);
		if (file_stream.is_open())
		{
			std::string value;
			CryptoPP::SHA1 sha1_cal;
			CryptoPP::FileSource fs(file_stream, true /* PumpAll */,
				new CryptoPP::HashFilter(sha1_cal,
					new CryptoPP::HexEncoder(
						new CryptoPP::StringSink(value),
						false
					) // HexEncoder
				) // HashFilter
			); // FileSource

			std::transform(sha1_.begin(), sha1_.end(), sha1_.begin(), tolower);
			if (0 == sha1_.compare(value))
			{
				need_download = false;
			}

			file_stream.close();
		}
	}

	std::weak_ptr<AppUpdate> weak_obj = (std::dynamic_pointer_cast<AppUpdate>)(this->shared_from_this());
	if (!need_download)
	{
		AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj, update_exe_path]()
		{
			auto obj = weak_obj.lock();
			if (!!obj)
			{
				obj->update_state_ = kStop;
				obj->update_download_success(update_exe_path, obj->app_name_, obj->version_, obj->note_);
			}
		}), true);
	}
	else
	{
		AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj]()
		{
			auto obj = weak_obj.lock();
			if (!!obj)
			{
				obj->update_state_ = kDownloadingUpdateFile;
				obj->network_dispatcher_->add_task(obj);
			}
		}), true);
	}
}

bool AppUpdate::on_before_schedule()
{
	if (update_state_ == kFetchingUpdateInfo)
	{
		easy().add<CURLOPT_WRITEFUNCTION>(AppUpdate::on_update_info_buffer_arrive);
		easy().add<CURLOPT_WRITEDATA>(this);
		easy().add<CURLOPT_URL>(mcfe::g_update_info_url);

		update_info_buffer_.clear();
	}
	else if (update_state_ == kDownloadingUpdateFile)
	{
		::SHCreateDirectory(0, PathService().appdata_path().c_str());
		update_file_ = _wfopen((PathService().appdata_path() + L"\\update.exe").c_str(), L"wb");
		if (update_file_ == nullptr)
			return false;

		easy().add<CURLOPT_URL>(url_.c_str());
		easy().add<CURLOPT_WRITEFUNCTION>(AppUpdate::on_file_buffer_arrive);
		easy().add<CURLOPT_WRITEDATA>(update_file_);
	}

	__super::on_before_schedule();
	return true;
}

size_t AppUpdate::on_file_buffer_arrive(void * ptr, size_t size, size_t nmemb, void * userdata)
{
	FILE* file_ptr = static_cast<FILE*>(userdata);
	if (file_ptr)
	{
		return fwrite(ptr, size, nmemb, file_ptr);
	}

	return size*nmemb;
}

size_t AppUpdate::on_update_info_buffer_arrive(void * ptr, size_t size, size_t nmemb, void * userdata)
{
	AppUpdate* pthis = static_cast<AppUpdate*>(userdata);
	if (pthis)
	{
		pthis->update_info_buffer_.append((const char*)ptr, size*nmemb);
	}

	return size*nmemb;
	
}

void AppUpdate::on_network_task_state_changed(std::shared_ptr<NetworkTask>, State state)
{
	if (state == NetworkTask::kStop)
	{
		DataReport *date_report = DataReport::global();
		if (update_state_ == kFetchingUpdateInfo)
		{
			if (is_cancelled())
			{
				date_report->report_event(FETCH_UPDATE_INFO_FAILED_EVENT, 1);
				update_state_ = kStop;
				return;
			}
			else if (curl_result() != CURLE_OK)
			{
				date_report->report_event(FETCH_UPDATE_INFO_FAILED_EVENT, 2);
				update_state_ = kStop;
				return;
			}

			rapidjson::Document doc;
			doc.Parse(update_info_buffer_.c_str(), update_info_buffer_.length());
			if (doc.IsObject())
			{
				std::string name;
				JsonGetStringMemberValue(doc, "name", name);
				if (!name.empty())
				{
					name = UrlDecode(name);
					std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
					app_name_ = converter.from_bytes(name);
				}

				std::string note;
				JsonGetStringMemberValue(doc, "note", note);
				if (!note.empty())
				{
					note = UrlDecode(note);
					std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
					note_ = converter.from_bytes(note);
				}

				std::string version;
				if (JsonGetStringMemberValue(doc, "version", version)
					&& JsonGetStringMemberValue(doc, "download", url_)
					&& JsonGetStringMemberValue(doc, "sha1", sha1_))
				{
					version_ = std::wstring(version.begin(), version.end());
					wchar_t path[1024] = { 0 };
					::GetModuleFileNameW(nullptr, path, 1024);
					DWORD main_ver = 0, sub_ver = 0, build_ver = 0, rev_ver = 0;
					if (FileVersion().get_file_version(path, &main_ver, &sub_ver, &build_ver, &rev_ver))
					{
						auto split_strs = split_string(version, ".");
						if (split_strs.size() == 3)
						{
							DWORD new_main_ver = atoi(split_strs[0].c_str());
							DWORD new_sub_ver = atoi(split_strs[1].c_str());
							DWORD new_build_ver = atoi(split_strs[2].c_str());

							if (new_main_ver > main_ver
								|| (new_main_ver == main_ver && new_sub_ver > sub_ver)
								|| (new_main_ver == main_ver && new_sub_ver == sub_ver && new_build_ver>build_ver))
							{
								date_report->report_event(NEED_UPDATE_APP_EVENT, main_ver, sub_ver, build_ver);
								check_update_exists_and_download();
								return;
							}
							
						}
					}
				}
			}
			else
			{
				date_report->report_event(FETCH_UPDATE_INFO_FAILED_EVENT, 3);
			}
			
			update_state_ = kStop;
		}
		else if (update_state_ == kDownloadingUpdateFile)
		{
			if (update_file_)
			{
				fclose(update_file_);
				update_file_ = nullptr;
			}
			else
			{
				update_state_ = kStop;
				return;
			}

			if (is_cancelled())
			{
				date_report->report_event(DOWNLOAD_UPDATE_PACK_FAILED_EVENT, 1);
				update_state_ = kStop;
				return;
			}
			else if (curl_result() != CURLE_OK)
			{
				date_report->report_event(DOWNLOAD_UPDATE_PACK_FAILED_EVENT, 2);
				update_state_ = kStop;
				return;
			}

			std::wstring update_exe_path = PathService().appdata_path() + L"\\update.exe";
			std::ifstream file_stream(update_exe_path, std::ifstream::binary | std::ifstream::in);
			if (file_stream.is_open())
			{
				std::string value;
				CryptoPP::SHA1 sha1;
				CryptoPP::FileSource fs(file_stream, true /* PumpAll */,
					new CryptoPP::HashFilter(sha1,
						new CryptoPP::HexEncoder(
							new CryptoPP::StringSink(value),
							false
							) // HexEncoder
						) // HashFilter
					); // FileSource

				std::transform(sha1_.begin(), sha1_.end(), sha1_.begin(), tolower);
				if (0 == sha1_.compare(value))
				{
					std::weak_ptr<AppUpdate> weak_obj = (std::dynamic_pointer_cast<AppUpdate>)(this->shared_from_this());
					AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj, update_exe_path]()
					{
						auto obj = weak_obj.lock();
						if (!!obj)
						{
							obj->update_state_ = kStop;
							obj->update_download_success(update_exe_path, obj->app_name_, obj->version_, obj->note_);
						}
					}), true);

					return;
				}
				else
				{
					date_report->report_event(DOWNLOAD_UPDATE_PACK_FAILED_EVENT, 3);
				}

				file_stream.close();
			}

			update_state_ = kStop;
		}
	}
}

bool AppUpdate::on_after_schedule()
{
	__super::on_after_schedule();

	if (update_state_ == kFetchingUpdateInfo)
	{
		long http_rescode = 0;
		curl_easy_getinfo(easy().get_curl(), CURLINFO_RESPONSE_CODE, &http_rescode);
		if (http_rescode == 304)
		{
			auto entry = DownloadCacheManager::GetInstance()->cache_entry_for_url(mcfe::g_update_info_url);
			update_info_buffer_.clear();
			if (!DownloadCacheManager::GetInstance()->read_cache_file(entry, update_info_buffer_))
			{
				DCHECK(0);
			}
		}
		else if (http_rescode == 200)
		{
			std::string last_modified_value = last_modified();
			if (!last_modified_value.empty())
			{
				auto entry = DownloadCacheManager::GetInstance()->cache_entry_for_url(mcfe::g_update_info_url);
				if (!DownloadCacheManager::GetInstance()->update_cache_file(entry, update_info_buffer_, last_modified_value))
					DCHECK(0);
			}
		}
		else
		{
			LOG(ERROR) << "get update list with http code:" << http_rescode;
		}
	}

	return true;
}

struct curl_slist* AppUpdate::on_set_header(struct curl_slist *header)
{
	if (update_state_ == kFetchingUpdateInfo)
	{
		auto entry = DownloadCacheManager::GetInstance()->cache_entry_for_url(mcfe::g_update_info_url);
		auto if_modified_since = DownloadCacheManager::GetInstance()->get_if_modified_since(entry);
		if (!if_modified_since.empty())
		{
			if_modified_since = "If-Modified-Since: " + if_modified_since;
			header = curl_slist_append(header, if_modified_since.c_str());
		}
	}
	header = __super::on_set_header(header);
	return header;
}
