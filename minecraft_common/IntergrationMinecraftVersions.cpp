#include "IntergrationMinecraftVersions.h"
#include "MinecraftCommon.h"
#include "glog/logging.h"
#include "base/NetworkTaskDispatcher.h"
#include "base/PathService.h"
#include "base/at_exit_manager.h"
#include "base/DownloadCacheManager.h"
#include "base/JsonHelper.h"

#include <shlobj.h>
#include <codecvt>

IntegrationMinecraftVersions::IntegrationMinecraftVersions()
{
	easy().add<CURLOPT_URL>(mcfe::g_integration_minecraft_version_json_url);
	easy().add<CURLOPT_FOLLOWLOCATION>(1L);

	this->state_changed.connect(this, &IntegrationMinecraftVersions::on_network_task_state_changed);
}

IntegrationMinecraftVersions::~IntegrationMinecraftVersions()
{

}

void IntegrationMinecraftVersions::start_update_lists()
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

void IntegrationMinecraftVersions::stop_update_lists()
{
	set_cancel(true);
}

size_t IntegrationMinecraftVersions::write_callback(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	IntegrationMinecraftVersions *pthis = static_cast<IntegrationMinecraftVersions*>(userdata);
	auto sz = size*nmemb;
	if (pthis)
	{
		pthis->buffer_.append((char*)ptr, sz);
	}

	return sz;
}

bool IntegrationMinecraftVersions::parse_content()
{
	rapidjson::Document mc_ver_list_obj;
	mc_ver_list_obj.Parse(buffer_.c_str(), buffer_.length());
	if (!mc_ver_list_obj.IsArray())
	{
		LOG(ERROR) << "can not parse ver list for new mc ver list file.";
		return false;
	}

	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
	for (auto value = mc_ver_list_obj.Begin(); value != mc_ver_list_obj.End(); value++)
	{
		if(!value->IsObject())
			continue;
		std::shared_ptr<VersionInfo> version(new VersionInfo);

		std::string name, desc;
		JsonGetStringMemberValue(*value, "name", name);
		if(name.empty())
			continue;
		JsonGetStringMemberValue(*value, "desc", desc);
		if(desc.empty())
			continue;

		uint32_t size = 0;
		JsonGetUIntMemberValue(*value, "size", size);
		if (size == 0)
		{
			continue;
		}
		version->size = size;

		JsonGetStringMemberValue(*value, "sha1", version->sha1);
		if(version->sha1.empty())
			continue;

		JsonGetStringMemberValue(*value, "profile", version->profile_url);
		if(version->profile_url.empty())
			continue;

		version->name = converter.from_bytes(name);
		version->desc = converter.from_bytes(desc);

		mc_versions_.push_back(version);
	}
	return true;
}

bool IntegrationMinecraftVersions::on_before_schedule()
{
	__super::on_before_schedule();

	easy().add<CURLOPT_WRITEFUNCTION>(IntegrationMinecraftVersions::write_callback);
	easy().add<CURLOPT_WRITEDATA>(this);

	buffer_.clear();

	return true;
}

bool IntegrationMinecraftVersions::on_after_schedule()
{
	AtExitManager at_exit;
	at_exit.register_callback(make_closure([this]() {NetworkTask::on_after_schedule(); }));

	if (is_cancelled())
		return true;
	else if (curl_result() != CURLE_OK)
	{
		return true;
	}

	long http_rescode = 0;
	curl_easy_getinfo(easy().get_curl(), CURLINFO_RESPONSE_CODE, &http_rescode);
	if (http_rescode == 304)
	{
		auto entry = DownloadCacheManager::GetInstance()->cache_entry_for_url(mcfe::g_integration_minecraft_version_json_url);
		buffer_.clear();
		if (!DownloadCacheManager::GetInstance()->read_cache_file(entry, buffer_))
		{
			DCHECK(0);
		}
	}
	else if (http_rescode == 200)
	{
		std::string last_modified_value = last_modified();
		if (!last_modified_value.empty())
		{
			auto entry = DownloadCacheManager::GetInstance()->cache_entry_for_url(mcfe::g_integration_minecraft_version_json_url);
			if (!DownloadCacheManager::GetInstance()->update_cache_file(entry, buffer_, last_modified_value))
				DCHECK(0);
		}
	}
	else
	{
		LOG(ERROR) << "get intergration list failed with http code:" << http_rescode;
	}

	if (parse_content())
	{
		update_success_ = true;
	}

	return true;
}

void IntegrationMinecraftVersions::on_network_task_state_changed(std::shared_ptr<NetworkTask>, State state)
{
	if (state == kStop)
	{
		std::weak_ptr<IntegrationMinecraftVersions> weak_obj = std::dynamic_pointer_cast<IntegrationMinecraftVersions>(this->shared_from_this());
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

struct curl_slist* IntegrationMinecraftVersions::on_set_header(curl_slist* header)
{
	auto entry = DownloadCacheManager::GetInstance()->cache_entry_for_url(mcfe::g_integration_minecraft_version_json_url);
	auto if_modified_since = DownloadCacheManager::GetInstance()->get_if_modified_since(entry);
	if (!if_modified_since.empty())
	{
		if_modified_since = "If-Modified-Since: " + if_modified_since;
		header = curl_slist_append(header, if_modified_since.c_str());
	}

	header = __super::on_set_header(header);
	return header;
}
