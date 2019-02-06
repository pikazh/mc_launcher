#pragma once

#include "MinecraftCommon.h"
#include "base/CommonDefine.h"
#include "document.h"
#include "base/sigslot.h"
#include "base/NetworkTaskDispatcher.h"
#include "DownloadGameFileTask.h"

#include <string>
#include <memory>
#include <map>
#include <set>
#include <list>

class MinecraftProfile;

class MinecraftDownload
	: public std::enable_shared_from_this<MinecraftDownload>
	, public sigslot::has_slots<>
{
	friend class MinecraftInstanceManager;
public:
	virtual ~MinecraftDownload();

protected:
	MinecraftDownload(const std::wstring base_dir, const std::wstring version_str, const std::string profile_url);
	DISABLE_COPY_AND_ASSIGN(MinecraftDownload)

public:
	bool start_download(bool download_assets);
	void stop_download();

	const std::wstring& base_dir()
	{
		return base_dir_;
	}

	const std::wstring& version_str()
	{
		return version_;
	}

	const std::string& profile_url()
	{
		return profile_url_;
	}

	bool is_downloading()
	{
		return downloading_;
	}

	const std::vector<std::shared_ptr<DownloadGameFileTask>>& dl_failed_task()
	{
		return dl_failed_task_;
	}

	uint32_t all_download_item_num() { return all_download_item_num_; }
	uint32_t downloaded_item_num() { return downloaded_item_num_; }

	sigslot::signal2<std::shared_ptr<MinecraftDownload>, bool> download_finished;
protected:
	void download_client_and_library_files();
	void download_profile_file();
	void download_assets_index_file();
	void download_assets_files();

	void on_download_profile_failed();
	void on_download_assets_index_failed();

	void on_profile_task_state_changed(std::shared_ptr<NetworkTask> task, NetworkTask::State state);
	void on_asset_index_file_task_state_changed(std::shared_ptr<NetworkTask> task, NetworkTask::State state);
	void on_client_and_lib_task_state_changed(std::shared_ptr<NetworkTask> task, NetworkTask::State state);
	void on_asset_files_task_state_changed(std::shared_ptr<NetworkTask> task, NetworkTask::State state);

	void emit_download_finished(bool success);
private:
	std::wstring base_dir_;
	std::wstring version_;
	std::string profile_url_;

	std::shared_ptr<DownloadGameFileTask> dl_profile_task_;
	std::map<uint32_t, std::shared_ptr<DownloadGameFileTask>> libraries_file_task_;
	std::shared_ptr<DownloadGameFileTask> asset_index_task_;
	std::map<uint32_t, std::shared_ptr<DownloadGameFileTask>> assets_file_task_;
	std::vector<std::shared_ptr<DownloadGameFileTask>> dl_failed_task_;
	std::shared_ptr<MinecraftProfile> dl_instance_;
	std::map<std::wstring, std::set<std::wstring>> virtual_assets_;

	bool downloading_ = false;

	bool download_assets_ = false;
	bool downloading_assets_ = false;

	uint32_t all_download_item_num_ = 0;
	uint32_t downloaded_item_num_ = 0;
};

