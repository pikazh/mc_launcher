#include "OfficialMinecraftVersions.h"
#include "curl/curl.h"
#include "curl_easy.h"
#include "curl_multi.h"
#include "MinecraftCommon.h"
#include "base/PathService.h"
#include "glog/logging.h"
#include "base/asynchronous_task_dispatch.h"
#include "base/JsonHelper.h"
#include "base/NetworkTaskDispatcher.h"
#include "base/at_exit_manager.h"
#include "base/DownloadCacheManager.h"

#include <iostream>
#include <shlobj.h>
#include <codecvt>
#include <shlobj.h>
#include <set>
#include <atlbase.h>
#include <atlconv.h>
#include <atlstr.h>
#include <io.h>

OfficialMinecraftVersions::OfficialMinecraftVersions()
{
	easy().add<CURLOPT_URL>(mcfe::g_official_minecraft_versions_json_url);
	easy().add<CURLOPT_FOLLOWLOCATION>(1L);

	this->state_changed.connect(this, &OfficialMinecraftVersions::on_network_task_state_changed);
}

OfficialMinecraftVersions::~OfficialMinecraftVersions()
{
}

void OfficialMinecraftVersions::start_update_lists()
{
	if (state() != kStop)
	{
		DCHECK(0);
		return;
	}

	set_cancel(false);
	update_success_ = false;
	mc_versions_.clear();

	NetworkTaskDispatcher::global()->add_task(this->shared_from_this());
}

bool OfficialMinecraftVersions::parse_file(const std::wstring &file)
{
	std::ifstream in_file_stream(file);
	if (!in_file_stream.is_open())
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_cvt;
		static std::string file_name = utf8_cvt.to_bytes(file);
		LOG(ERROR) << "can not open " << file_name.c_str() << " for reading ver list.";
		return false;
	}

	in_file_stream.seekg(0, in_file_stream.end);
	auto length = in_file_stream.tellg();
	in_file_stream.seekg(0, in_file_stream.beg);
	char* buf = new char[length];
	in_file_stream.read(buf, length);
	std::unique_ptr<char[]> auto_ptr(buf);
	
	rapidjson::Document mc_ver_list_obj;
	mc_ver_list_obj.Parse(buf, length);
	if (!mc_ver_list_obj.IsObject())
	{
		LOG(ERROR) << "can not parse ver list for new mc ver list file.";
		return false;
	}

	auto it = mc_ver_list_obj.FindMember("versions");
	DCHECK(it != mc_ver_list_obj.MemberEnd());
	if (it == mc_ver_list_obj.MemberEnd())
	{
		LOG(ERROR) << "can not versions member in mc list json obj";
		return false;
	}
	rapidjson::Value &versions = it->value;
	DCHECK(versions.IsArray());
	if (!versions.IsArray())
	{
		LOG(ERROR) << "what? mc list format has changed?";
		return false;
	}

	for (auto value = versions.Begin(); value != versions.End(); value++)
	{
		std::shared_ptr<VersionInfo> version(new VersionInfo);
		std::string type;
		if (JsonGetStringMemberValue(*value, "type", type))
		{
			if (stricmp("release", type.c_str()) == 0)
				version->type = kRelease;
			else if (stricmp("snapshot", type.c_str()) == 0)
				version->type = kSnapshot;
			else if (stricmp("old_beta", type.c_str()) == 0)
				version->type = kOldAlpha;
			else if (stricmp("old_alpha", type.c_str()) == 0)
				version->type = kOldAlpha;
			else
				DCHECK(0);
		}
		else
		{
			continue;
		}

		if (JsonGetStringMemberValue(*value, "id", version->id)
			&& JsonGetStringMemberValue(*value, "url", version->url)
			&& JsonGetStringMemberValue(*value, "releaseTime", version->release_time))
		{
			auto index = version->release_time.find('T');
			if (index != std::string::npos)
				version->release_time = version->release_time.substr(0, index);

			mc_versions_.push_back(version);
		}
		
	}

	return true;
}

bool OfficialMinecraftVersions::on_after_schedule()
{
	__super::on_after_schedule();

	AtExitManager at_exit;
	at_exit.register_callback(make_closure([this]() 
	{
		if (save_file_)
		{
			fclose(save_file_);
			save_file_ = nullptr;
		}
	}));

	if (!is_cancelled() && curl_result() == CURLE_OK)
	{
		long http_rescode = 0;
		curl_easy_getinfo(easy().get_curl(), CURLINFO_RESPONSE_CODE, &http_rescode);
		if (http_rescode == 304)
		{
			auto entry = DownloadCacheManager::GetInstance()->cache_entry_for_url(mcfe::g_official_minecraft_versions_json_url);

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
				auto entry = DownloadCacheManager::GetInstance()->cache_entry_for_url(mcfe::g_official_minecraft_versions_json_url);
				if (!DownloadCacheManager::GetInstance()->update_cache_file(entry, save_file_, last_modified_value))
					DCHECK(0);
			}
		}
		else
		{
			LOG(ERROR) << "get mc game list failed with http code:" << http_rescode;
		}

		fclose(save_file_);
		save_file_ = nullptr;

		if (parse_file(PathService().appdata_path() + L"\\mc_vers"))
		{
			update_success_ = true;
		}
	}

	return true;
}

bool OfficialMinecraftVersions::on_before_schedule()
{
	::SHCreateDirectory(0, PathService().appdata_path().c_str());
	save_file_ = _wfopen((PathService().appdata_path() + L"\\mc_vers").c_str(), L"wb+");
	if (save_file_ == nullptr)
		return false;

	easy().add<CURLOPT_WRITEFUNCTION>(OfficialMinecraftVersions::write_callback);
	easy().add<CURLOPT_WRITEDATA>(save_file_);

	__super::on_before_schedule();
	return true;
}

size_t OfficialMinecraftVersions::write_callback(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	FILE* file_ptr = static_cast<FILE*>(userdata);
	if (file_ptr)
	{
		return fwrite(ptr, size, nmemb, file_ptr);
	}
	DCHECK(0);
	return size*nmemb;
}

void OfficialMinecraftVersions::on_network_task_state_changed(std::shared_ptr<NetworkTask>, State state)
{
	if (state == kStop)
	{
		std::weak_ptr<OfficialMinecraftVersions> weak_obj = std::dynamic_pointer_cast<OfficialMinecraftVersions>(this->shared_from_this());
		AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj]()
		{
			auto obj = weak_obj.lock();
			if (!!obj)
			{
				obj->update_finish(obj->update_success_);
			}
		}), true);
	}
	
}

void OfficialMinecraftVersions::stop_update_lists()
{
	set_cancel(true);
}

struct curl_slist* OfficialMinecraftVersions::on_set_header(struct curl_slist* h)
{
	auto entry = DownloadCacheManager::GetInstance()->cache_entry_for_url(mcfe::g_official_minecraft_versions_json_url);
	auto if_modified_since = DownloadCacheManager::GetInstance()->get_if_modified_since(entry);
	if (!if_modified_since.empty())
	{
		if_modified_since = "If-Modified-Since: " + if_modified_since;
		h = curl_slist_append(h, if_modified_since.c_str());
	}
	h = __super::on_set_header(h);
	return h;
}
