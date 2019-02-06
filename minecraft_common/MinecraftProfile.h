#pragma once

#include "document.h"
#include "MinecraftCommon.h"
#include "glog/logging.h"

#include <memory>
#include <string>
#include <vector>
#include <shlwapi.h>

//请先确保client profile json文件存在在version_str目录下，否则解析会失败
class MinecraftProfile
	: public std::enable_shared_from_this<MinecraftProfile>
{
public:
	MinecraftProfile(const std::wstring &base_dir, const std::wstring &version_str);
	~MinecraftProfile();

public:
	const std::wstring& base_dir()
	{
		return base_dir_;
	}

	const std::wstring& version_str()
	{
		return version_str_;
	}

	const std::wstring& client_dir()
	{
		return client_dir_;
	}

	const std::wstring assets_dir()
	{
		if (assets().compare("legacy") == 0)
		{
			std::wstring path = base_dir_ + L"\\assets\\virtual\\legacy";
			if (::PathIsDirectoryW(path.c_str()) && !::PathIsDirectoryEmpty(path.c_str()))
				return path;
			else
				return base_dir_ + L"\\assets";
		}
		else
			return base_dir_ + L"\\assets";
	}

	const std::string& main_class()
	{
		if (!main_class_.empty())
			return main_class_;
		else if(!!inheritsFrom_)
		{
			return inheritsFrom_->main_class();
		}

		DCHECK(0);
		return main_class_;
	}

	const std::string& launch_argument()
	{
		if (!arguments_.empty())
			return arguments_;
		else if (!!inheritsFrom_)
			return inheritsFrom_->launch_argument();
		
		DCHECK(0);
		return arguments_;
	}

	bool has_forge()
	{
		if (!arguments_.empty())
		{
			auto index = arguments_.find(".FMLTweaker");
			if (index != std::string::npos)
			{
				return true;
			}
		}

		return false;
	}

	bool has_liteloader()
	{
		if (!arguments_.empty())
		{
			auto index = arguments_.find(".LiteLoaderTweaker");
			if (index != std::string::npos)
			{
				return true;
			}
		}

		return false;
	}

	const std::string& assets()
	{
		if (!assets_.empty())
			return assets_;
		else if(!!inheritsFrom_)
			return inheritsFrom_->assets();

		DCHECK(0);
		return assets_;
	}

	const std::string& id()
	{
		return id_;
	}

	const std::string& jar()
	{
		return jar_;
	}

	std::shared_ptr<MinecraftProfile> inheritsFrom()
	{
		return inheritsFrom_;
	}

	mcfe::GameFile& asset_index_file()
	{
		if (asset_index_json_ && asset_index_json_->IsObject())
			return asset_index_file_;
		else if (!!inheritsFrom_)
			return inheritsFrom_->asset_index_file();

		DCHECK(0);
		return asset_index_file_;
	}

	mcfe::GameFile& client_file()
	{
		if (client_file_.download_url.empty()
			&& (client_file_.file_path.empty() || !::PathFileExistsW(client_file_.file_path.c_str()))
			&& !!inheritsFrom_
			)
			return inheritsFrom_->client_file();

		return client_file_;
	}

	const std::vector<std::shared_ptr<mcfe::GameFile>> library_files()
	{
		auto libs = library_files_;
		if (!!inheritsFrom_)
		{
			auto parent_libs = inheritsFrom_->library_files();
			libs.insert(libs.end(), parent_libs.begin(), parent_libs.end());
		}
			
		return libs;
	}
	
	const std::vector<std::shared_ptr<mcfe::GameFile>>& asset_files()
	{
		if (asset_index_json_ && asset_index_json_->IsObject())
			return asset_files_;
		else if (!!inheritsFrom_)
			return inheritsFrom_->asset_files();

		DCHECK(0);
		return asset_files_;
	}

	bool assets_virtual()
	{
		return assets_virtual_;
	}

	const std::string &version_type()
	{
		if (version_type_.empty() && !!inheritsFrom_)
			return inheritsFrom_->version_type();

		return version_type_;
	}

	bool load_game_profile_file(bool deep_parse=false);
	bool load_asset_profile_file();

	std::wstring sid();

protected:
	
	bool parse_game_file();

	bool load_parent_game_profile_file(bool deep_parse);

private:
	rapidjson::Document game_profile_json_;
	rapidjson::Document asset_profile_json_;
	rapidjson::Value *asset_index_json_ = nullptr;
	rapidjson::Value *client_json_ = nullptr;
	rapidjson::Value *libraries_ = nullptr;
	std::string inhert_from_string_;

	std::wstring base_dir_;
	std::wstring version_str_;
	std::wstring client_dir_;

	std::string version_type_;
	std::string main_class_;
	std::string arguments_;
	std::string id_;
	std::string assets_;

	std::shared_ptr<MinecraftProfile> inheritsFrom_;
	std::string jar_;

	mcfe::GameFile asset_index_file_;
	mcfe::GameFile client_file_;
	std::vector<std::shared_ptr<mcfe::GameFile>> library_files_;
	std::vector<std::shared_ptr<mcfe::GameFile>> asset_files_;
	bool assets_virtual_ = false;
};