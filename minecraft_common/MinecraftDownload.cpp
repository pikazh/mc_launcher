#include "MinecraftDownload.h"
#include "MinecraftProfile.h"
#include "base/StringHelper.h"
#include "base/FileHelper.h"

#include <shlobj.h>
#include <shlwapi.h>
#include <codecvt>
#include <thread>

MinecraftDownload::MinecraftDownload(const std::wstring base_dir, const std::wstring version_str, const std::string profile_url)
{
	base_dir_ = base_dir;
	version_ = version_str;
	profile_url_ = profile_url;

}

MinecraftDownload::~MinecraftDownload()
{
}

bool MinecraftDownload::start_download(bool download_assets)
{
	if (downloading_)
	{
		return true;
	}
	downloading_ = true;

	download_assets_ = download_assets;
	downloading_assets_ = false;
	dl_profile_task_.reset();
	asset_index_task_.reset();
	dl_failed_task_.clear();
	libraries_file_task_.clear();
	assets_file_task_.clear();
	all_download_item_num_ = 0;
	downloaded_item_num_ = 0;

	download_profile_file();

	return true;
}

void MinecraftDownload::stop_download()
{
	if (!!dl_profile_task_)
		dl_profile_task_->set_cancel(true);
	if (!!asset_index_task_)
		asset_index_task_->set_cancel(true);
	for (auto &it : libraries_file_task_)
		it.second->set_cancel(true);
	for (auto &it : assets_file_task_)
		it.second->set_cancel(true);
}

void MinecraftDownload::download_profile_file()
{
	dl_profile_task_ = std::make_shared<DownloadGameFileTask>();
	dl_profile_task_->set_download_url(profile_url_);
	std::wstring old_file = base_dir_ + L"\\versions\\" + version_ + L"\\" + version_ + L".json_tmp";
	::DeleteFileW(old_file.c_str());
	dl_profile_task_->set_download_file_path(old_file.c_str());
	all_download_item_num_ += 1;
	dl_profile_task_->state_changed.connect(this, &MinecraftDownload::on_profile_task_state_changed);
	NetworkTaskDispatcher::global()->add_task(dl_profile_task_);
}

void MinecraftDownload::on_profile_task_state_changed(std::shared_ptr<NetworkTask> task, NetworkTask::State state)
{
	if (state != NetworkTask::kStop)
		return;

	downloaded_item_num_ += 1;
	if (!dl_profile_task_->download_success())
	{
		on_download_profile_failed();
	}
	else
	{
		std::wstring old_file = base_dir_ + L"\\versions\\" + version_ + L"\\" + version_ + L".json_tmp";
		std::wstring new_file = base_dir_ + L"\\versions\\" + version_ + L"\\" + version_ + L".json";
		if (::CopyFileW(old_file.c_str(), new_file.c_str(), FALSE))
			::DeleteFileW(old_file.c_str());

		dl_instance_ = std::make_shared<MinecraftProfile>(base_dir_, version_);
		if (dl_instance_->load_game_profile_file(true))
		{
			download_client_and_library_files();
			if (download_assets_)
				download_assets_index_file();
		}
		else
		{
			//DCHECK(0);
			on_download_profile_failed();
		}
	}

	dl_profile_task_.reset();
}

void MinecraftDownload::on_asset_index_file_task_state_changed(std::shared_ptr<NetworkTask> task, NetworkTask::State state)
{
	if (state != NetworkTask::kStop)
		return;

	downloaded_item_num_ += 1;
	if (!asset_index_task_->download_success())
	{
		on_download_assets_index_failed();
	}
	else
	{
		if (dl_instance_->load_asset_profile_file())
		{
			download_assets_files();
		}
		else
		{
			DCHECK(0);
			on_download_assets_index_failed();
		}
	}

	asset_index_task_.reset();

}

void MinecraftDownload::on_client_and_lib_task_state_changed(std::shared_ptr<NetworkTask> task, NetworkTask::State state)
{
	if (state != NetworkTask::kStop)
		return;

	auto it = libraries_file_task_.find(task->task_id());
	if (it != libraries_file_task_.end())
	{
		downloaded_item_num_ += 1;
		auto dl_task = it->second;
		libraries_file_task_.erase(it);
		if (!dl_task->download_success())
		{
			dl_failed_task_.push_back(dl_task);
		}
	}
	else
	{
		DCHECK(0);
	}

	if ((!download_assets_ && libraries_file_task_.empty())
		|| (download_assets_ && downloading_assets_ && libraries_file_task_.empty() && assets_file_task_.empty()))
	{
		emit_download_finished(dl_failed_task_.empty());
	}
}

void MinecraftDownload::on_asset_files_task_state_changed(std::shared_ptr<NetworkTask> task, NetworkTask::State state)
{
	if (state != NetworkTask::kStop)
		return;

	auto it = assets_file_task_.find(task->task_id());
	if (it != assets_file_task_.end())
	{
		downloaded_item_num_ += 1;
		auto dl_task = it->second;
		assets_file_task_.erase(it);
		if (!dl_task->download_success())
		{
			dl_failed_task_.push_back(dl_task);
		}
		else
		{
#ifdef _DEBUG
			if (!virtual_assets_.empty())
#endif
			{
				auto it = virtual_assets_.find(dl_task->download_file_path());
				if (it != virtual_assets_.end())
				{
					std::wstring src_file = it->first;
					std::set<std::wstring> dst_files = it->second;
					std::thread copy_file_thread([src_file, dst_files]()
					{
						for (auto dst_file : dst_files)
						{
							wstring_replace_all(dst_file, L"/", L"\\");
							std::ifstream src_stream(src_file, std::ifstream::in | std::ifstream::binary);
							std::ifstream dst_stream(dst_file, std::ifstream::in | std::ifstream::binary);
							if (src_stream.is_open() && dst_stream.is_open() && FileHelper().equal_file_content(src_stream, dst_stream))
								return;

							if (src_stream.is_open())
								src_stream.close();
							if (dst_stream.is_open())
								dst_stream.close();

							std::wstring dst_dir = dst_file;
							*::PathFindFileNameW((LPWSTR)dst_dir.c_str()) = 0;
							auto ret1 = ::SHCreateDirectory(0, dst_dir.c_str());
							if (!CopyFileW(src_file.c_str(), dst_file.c_str(), FALSE))
							{
								auto err = ::GetLastError();
								std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
								std::string src, dst;
								dst = converter.to_bytes(dst_dir);
								src = converter.to_bytes(src_file);
								LOG(ERROR) << "copy " << src << " to " << dst << "failed with err:" << err;
							}
						}

					});
					copy_file_thread.detach();

					virtual_assets_.erase(it);
				}
#ifdef _DEBUG
				else
				{
					DCHECK(0);
				}
#endif
			}

		}
	}
	else
	{
		DCHECK(0);
	}

	if ((!download_assets_ && libraries_file_task_.empty())
		|| (download_assets_ && downloading_assets_ && libraries_file_task_.empty() && assets_file_task_.empty()))
	{
		emit_download_finished(dl_failed_task_.empty());
	}
}

void MinecraftDownload::download_client_and_library_files()
{
	auto &client = dl_instance_->client_file();

	if (!client.download_url.empty())
	{
		auto lib_task = std::make_shared<DownloadGameFileTask>();
		lib_task->set_download_file_path(client.file_path);
		lib_task->set_download_url(client.download_url);
		lib_task->set_verify_file_size(client.file_size);
		if (!client.sha1.empty())
		{
			lib_task->set_verify_code(DownloadGameFileTask::kSha1, client.sha1);
		}
		all_download_item_num_ += 1;
		lib_task->state_changed.connect(this, &MinecraftDownload::on_client_and_lib_task_state_changed);
		libraries_file_task_[lib_task->task_id()] = lib_task;
		NetworkTaskDispatcher::global()->add_task(lib_task);
	}

	auto libs = dl_instance_->library_files();
	for (auto &lib : libs)
	{
		if (!lib->download_url.empty())
		{
			auto lib_task = std::make_shared<DownloadGameFileTask>();
			lib_task->set_download_file_path(lib->file_path);
			lib_task->set_download_url(lib->download_url);
			lib_task->set_verify_file_size(lib->file_size);
			if (!lib->sha1.empty())
			{
				lib_task->set_verify_code(DownloadGameFileTask::kSha1, lib->sha1);
			}

			all_download_item_num_ += 1;
			lib_task->state_changed.connect(this, &MinecraftDownload::on_client_and_lib_task_state_changed);
			libraries_file_task_[lib_task->task_id()] = lib_task;
			NetworkTaskDispatcher::global()->add_task(lib_task);
		}

	}
}

void MinecraftDownload::on_download_profile_failed()
{
	dl_failed_task_.push_back(dl_profile_task_);
	emit_download_finished(false);
}

void MinecraftDownload::download_assets_index_file()
{
	auto &assets_index_file = dl_instance_->asset_index_file();

	if (!assets_index_file.download_url.empty())
	{
		auto lib_task = std::make_shared<DownloadGameFileTask>();
		lib_task->set_download_file_path(assets_index_file.file_path);
		lib_task->set_download_url(assets_index_file.download_url);
		lib_task->set_verify_file_size(assets_index_file.file_size);
		if (!assets_index_file.sha1.empty())
		{
			lib_task->set_verify_code(DownloadGameFileTask::kSha1, assets_index_file.sha1);
		}
		all_download_item_num_ += 1;
		asset_index_task_ = lib_task;
		lib_task->state_changed.connect(this, &MinecraftDownload::on_asset_index_file_task_state_changed);
		NetworkTaskDispatcher::global()->add_task(asset_index_task_);
	}
}

void MinecraftDownload::on_download_assets_index_failed()
{
	dl_failed_task_.push_back(asset_index_task_);
	emit_download_finished(false);

}

void MinecraftDownload::download_assets_files()
{
	auto &libs = dl_instance_->asset_files();
	bool virtual_assets = dl_instance_->assets_virtual();

	//资源会存在重复的情况。。。
	std::set<std::wstring> asset_paths;
	for (auto &lib : libs)
	{
		if (!asset_paths.count(lib->file_path) && !lib->download_url.empty())
		{
			asset_paths.insert(lib->file_path);

			auto lib_task = std::make_shared<DownloadGameFileTask>();
			lib_task->set_download_file_path(lib->file_path);
			lib_task->set_download_url(lib->download_url);
			lib_task->set_verify_file_size(lib->file_size);
			if (!lib->sha1.empty())
			{
				lib_task->set_verify_code(DownloadGameFileTask::kSha1, lib->sha1);
			}
			lib_task->enable_verify(false);//神他妈的现在发现有时候下载成功的资源就是验证不通过，翻墙就ok了，http就是不靠谱。。。所以这里关掉资源的验证
			all_download_item_num_ += 1;
			assets_file_task_[lib_task->task_id()] = lib_task;
			lib_task->state_changed.connect(this, &MinecraftDownload::on_asset_files_task_state_changed);
			NetworkTaskDispatcher::global()->add_task(lib_task);
		}

		if (virtual_assets)
		{
			DCHECK((lib->flag & FILE_FLAG_VIRTUAL_GAME_ASSETS) != 0);
			std::shared_ptr<mcfe::LegacyAssetFile> legace_asset_file = std::static_pointer_cast<mcfe::LegacyAssetFile>(lib);
			virtual_assets_[legace_asset_file->file_path].insert(legace_asset_file->virtual_file_path);
		}
	}
	downloading_assets_ = true;
}

void MinecraftDownload::emit_download_finished(bool success)
{
	std::weak_ptr<MinecraftDownload> weak_obj = this->shared_from_this();
	AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj, success]()
	{
		auto obj = weak_obj.lock();
		if (!!obj)
		{
			obj->downloading_ = false;
			obj->download_finished(obj, success);
		}
	}), true);

}
