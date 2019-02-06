#pragma once

#include "base/sigslot.h"
#include "MinecraftDownload.h"

#include <string>
#include <list>
#include <map>
#include <memory>
#include <mutex>


class MinecraftProfile;

class MinecraftInstanceManager
	: public std::enable_shared_from_this<MinecraftInstanceManager>
	, public sigslot::has_slots<>
{
public:
	static MinecraftInstanceManager* GetInstance();
	virtual ~MinecraftInstanceManager();

	struct MinecraftInstanceItem
	{
		std::wstring base_dir;
		std::wstring version_str;
		std::wstring note;
	};

protected:
	MinecraftInstanceManager();
	
public:
	sigslot::signal1<std::shared_ptr<MinecraftInstanceItem>> instances_added;
	sigslot::signal1<std::shared_ptr<MinecraftInstanceItem>> instances_removed;
	sigslot::signal0<> instances_all_removed;
	sigslot::signal1<std::shared_ptr<MinecraftDownload>> download_inst_added;
public:
	void check_and_add_minecraft_instance(const std::wstring &base_dir);
	void check_and_add_minecraft_instance(const std::wstring &base_dir, const std::wstring &version_str, const std::wstring &note = L"");
	void set_default_instance(const std::wstring &base_dir, const std::wstring &version_str);
	void modify_minecraft_instance_info(MinecraftInstanceItem &item);
	std::shared_ptr<MinecraftInstanceItem> default_local_inst() { std::lock_guard<std::recursive_mutex> l(lock_); return default_local_inst_; }
	void remove_minecraft_instance(const std::wstring &base_dir, const std::wstring &version_str);
	void remove_all_instance();
	void remove_download_instance(std::shared_ptr<MinecraftDownload> inst);

	std::map<std::wstring, std::map<std::wstring, std::shared_ptr<MinecraftInstanceItem>>> local_instances();
	std::map<std::wstring, std::map<std::wstring, std::shared_ptr<MinecraftDownload>>> downloading_instances();
	std::shared_ptr<MinecraftDownload> download_mc_instance(const std::wstring &base_dir, const std::wstring &version_str, const std::string &profile_url, bool *is_exist_before = nullptr);

protected:
	void add_minecraft_instance(const std::wstring &base_dir, const std::wstring &version_str, std::shared_ptr<MinecraftInstanceItem> &item);
private:
	std::shared_ptr<MinecraftInstanceItem> default_local_inst_;
	std::map<std::wstring, std::map<std::wstring, std::shared_ptr<MinecraftInstanceItem>>> local_insts_;
	std::map<std::wstring, std::map<std::wstring, std::shared_ptr<MinecraftDownload>>> downloading_insts_;
	std::recursive_mutex lock_;
};

