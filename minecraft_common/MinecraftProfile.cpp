#include "MinecraftProfile.h"
#include "MinecraftCommon.h"
#include "base\StringHelper.h"
#include "base\JsonHelper.h"
#include "glog\logging.h"
#include "sha.h"
#include "cryptlib.h"
#include "hex.h"
#include "files.h"
#include "base\SystemHelper.h"
#include "base\system_info.h"

#include <fstream>
#include <sys\stat.h>
#include <windows.h>
#include <codecvt>
#include <algorithm>
#include <atlstr.h>
#include <atlconv.h>

MinecraftProfile::MinecraftProfile(const std::wstring &base_dir, const std::wstring &version_str)
{
	CHECK(!version_str.empty());
	base_dir_ = base_dir;
	version_str_ = version_str;
	wstring_trim_right(base_dir_, L'\\');
	CHECK(!base_dir_.empty());
	client_dir_ = base_dir_ + L"\\versions\\" + version_str;
}

MinecraftProfile::~MinecraftProfile()
{

}

bool MinecraftProfile::parse_game_file()
{
	library_files_.clear();
	CHECK(libraries_ && libraries_->IsArray());

	if (client_json_ && client_json_->IsObject())
	{
		JsonGetStringMemberValue(*client_json_, "url", client_file_.download_url);
		JsonGetStringMemberValue(*client_json_, "sha1", client_file_.sha1);
		JsonGetUIntMemberValue(*client_json_, "size", client_file_.file_size);
	}
	client_file_.type = mcfe::kTypeClient;
	client_file_.file_path = client_dir_ + L"\\" + std::wstring(id_.begin(), id_.end()) + L".jar";
	client_file_.flag = 0;

	bool has_error = false;
	bool is_x64_sys = SystemInfo().is_x64_system();
	auto array = libraries_->GetArray();
	for (auto value = array.begin(); value != array.end(); value++)
	{
		std::shared_ptr<mcfe::GameFile> lib = std::make_shared<mcfe::GameFile>();

		std::string lib_name;
		if (!JsonGetStringMemberValue(*value, "name", lib_name))
		{
			has_error = true;
			continue;
		}

		auto package_end_index = lib_name.find(':');
		if (package_end_index == 0 || package_end_index == std::string::npos)
		{
			has_error = true;
			continue;
		}

		std::string package = lib_name.substr(0, package_end_index);
		auto name_end_index = lib_name.find(':', package_end_index + 1);
		if (name_end_index == 0 || name_end_index == std::string::npos)
		{
			has_error = true;
			continue;
		}

		std::string name = lib_name.substr(package_end_index + 1, name_end_index - package_end_index - 1);

		auto version_end_index = lib_name.find(':', name_end_index + 1);
		DCHECK(version_end_index == std::string::npos);
		if (version_end_index != std::string::npos)
		{
			has_error = true;
			continue;
		}

		std::string version = lib_name.substr(name_end_index + 1);

		std::string split_package;
		while (true)
		{
			auto index = package.find('.');
			if (index == std::string::npos)
			{
				split_package += package;
				split_package += '\\';
				break;
			}
			else
			{
				split_package += package.substr(0, index);
				split_package += '\\';
				package = package.substr(index + 1);
			}
		}

		std::string full_path = split_package;
		full_path += name;
		full_path += "\\";
		full_path += version;
		full_path += "\\";
		full_path += name;
		full_path += "-";
		full_path += version;
		full_path += ".jar";

		std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
		lib->file_path = base_dir_ + L"\\libraries\\" + converter.from_bytes(full_path);

		if (!inhert_from_string_.empty())
		{
			library_files_.push_back(lib);
			continue;
		}

		rapidjson::Value *rules = nullptr;
		bool skip_by_rule = false;
		if (JsonGetArrayMemberValue(*value, "rules", &rules))
		{
			auto array = rules->GetArray();
			for (auto val = array.begin(); val != array.end(); val++)
			{
				if (val->IsObject())
				{
					std::string action;
					if (JsonGetStringMemberValue(*val, "action", action))
					{
						std::transform(action.begin(), action.end(), action.begin(), ::tolower);
						if (action.compare("disallow") == 0)
						{
							rapidjson::Value *os = nullptr;
							std::string disallowos;
							if (JsonGetObjectMemberValue(*val, "os", &os) && JsonGetStringMemberValue(*os, "name", disallowos))
							{
								std::transform(disallowos.begin(), disallowos.end(), disallowos.begin(), ::tolower);
								if (disallowos.compare("windows") == 0)
								{
									skip_by_rule = true;
									break;
								}
							}
						}
						else if (action.compare("allow") == 0)
						{
							rapidjson::Value *os = nullptr;
							std::string allow;
							if (JsonGetObjectMemberValue(*val, "os", &os) && JsonGetStringMemberValue(*os, "name", allow))
							{
								std::transform(allow.begin(), allow.end(), allow.begin(), ::tolower);
								if (allow.compare("windows") != 0)
								{
									skip_by_rule = true;
									break;
								}
							}
						}
					}
				}
				else
				{
					DCHECK(0);
				}
			}
		}
		if (skip_by_rule)
			continue;

		rapidjson::Value *natives = nullptr;
		std::string native;
		bool is_native = false;
		if (JsonGetObjectMemberValue(*value, "natives", &natives))
		{
			bool ret = JsonGetStringMemberValue(*natives, "windows", native);
			DCHECK(ret && !native.empty());
			if (native.empty())
				continue;

			auto index = native.find("${arch}");
			if (index != std::string::npos)
			{
				native.replace(native.begin() + index, native.begin() + index + strlen("${arch}"), is_x64_sys ? "64" : "32");
			}

			is_native = true;
			lib->flag |= FILE_FLAG_NATIVE_LIBRARY;
		}

		rapidjson::Value *download_info = nullptr;
		if (is_native)
		{
			DCHECK(!native.empty());
			if (JsonGetObjectMemberValue(*value, "downloads", &download_info)
				&& JsonGetObjectMemberValue(*download_info, "classifiers", &download_info)
				&& JsonGetObjectMemberValue(*download_info, native.c_str(), &download_info))
			{
				JsonGetStringMemberValue(*download_info, "url", lib->download_url);
				JsonGetStringMemberValue(*download_info, "sha1", lib->sha1);
				JsonGetUIntMemberValue(*download_info, "size", lib->file_size);
			}
			
			auto index = lib->file_path.find_last_of('.');
			DCHECK(index != std::string::npos);
			if (index != std::string::npos)
			{
				auto temp = lib->file_path.substr(0, index);
				temp += L"-";
				temp += std::wstring(native.begin(), native.end());
				temp += lib->file_path.substr(index);
				lib->file_path = temp;
				//DCHECK(::PathFileExistsW(lib->file_path.c_str()));
				library_files_.push_back(lib);
			}
		}
		else
		{
			if (JsonGetObjectMemberValue(*value, "downloads", &download_info)
				&& JsonGetObjectMemberValue(*download_info, "artifact", &download_info))
			{
				JsonGetStringMemberValue(*download_info, "url", lib->download_url);
				JsonGetStringMemberValue(*download_info, "sha1", lib->sha1);
				JsonGetUIntMemberValue(*download_info, "size", lib->file_size);
			}

			library_files_.push_back(lib);
		}

	}

	return !has_error;
}

bool MinecraftProfile::load_game_profile_file(bool deep_parse)
{
	std::wstring full_profile_json_path = client_dir_ + L"\\" + version_str_ + L".json";

	std::ifstream file_stream(full_profile_json_path);
	if (!file_stream.is_open())
		return false;

	uint32_t size = file_stream.seekg(0, std::ifstream::end).tellg();
	file_stream.seekg(0, std::ifstream::beg);
	std::string buf(size + 1, '\0');
	file_stream.read((char*)buf.data(), size);

	int index = 0;
	do
	{
		index = buf.find('\t', index);
		if (index != std::string::npos)
		{
			buf.replace(index, 1, "");
			size--;
		}
		else
			break;

	} while (true);

	game_profile_json_.Parse(buf.c_str(), size);
	if (!game_profile_json_.IsObject())
		return false;

	JsonGetStringMemberValue(game_profile_json_, "jar", jar_);

	JsonGetStringMemberValue(game_profile_json_, "inheritsFrom", inhert_from_string_);

	if (!JsonGetStringMemberValue(game_profile_json_, "mainClass", main_class_))
		return false;

	if (!JsonGetStringMemberValue(game_profile_json_, "minecraftArguments", arguments_))
		return false;

	if (!JsonGetStringMemberValue(game_profile_json_, "id", id_))
		return false;

	if (inhert_from_string_.empty() && !JsonGetStringMemberValue(game_profile_json_, "assets", assets_))
	{
		assets_ = "legacy";
	}

	JsonGetStringMemberValue(game_profile_json_, "type", version_type_);

	if (!JsonGetArrayMemberValue(game_profile_json_, "libraries", &libraries_))
		return false;

	if (inhert_from_string_.empty())
	{
		if (JsonGetObjectMemberValue(game_profile_json_, "assetIndex", &asset_index_json_))
		{
			JsonGetStringMemberValue(*asset_index_json_, "url", asset_index_file_.download_url);
			JsonGetStringMemberValue(*asset_index_json_, "sha1", asset_index_file_.sha1);
			JsonGetUIntMemberValue(*asset_index_json_, "size", asset_index_file_.file_size);

			std::string id;
			if (JsonGetStringMemberValue(*asset_index_json_, "id", id))
			{
				asset_index_file_.file_path = base_dir_ + L"\\assets\\indexes\\" + std::wstring(id.begin(), id.end()) + L".json";
			}
		}

		rapidjson::Value *downloads = nullptr;
		if (JsonGetObjectMemberValue(game_profile_json_, "downloads", &downloads))
		{
			JsonGetObjectMemberValue(*downloads, "client", &client_json_);
		}

	}
	else
	{
		if (!load_parent_game_profile_file(deep_parse))
		{
			return false;
		}
	}

	if (!deep_parse)
		return true;
	
	return parse_game_file();
}

bool MinecraftProfile::load_parent_game_profile_file(bool deep_parse)
{
	CHECK(!inhert_from_string_.empty());

	USES_CONVERSION;
	inheritsFrom_.reset(new MinecraftProfile(base_dir_, std::wstring(ATL::CA2W(inhert_from_string_.c_str()))));
	return inheritsFrom_->load_game_profile_file(deep_parse);
}

bool MinecraftProfile::load_asset_profile_file()
{
	asset_files_.clear();
	
	std::ifstream file_stream(asset_index_file_.file_path);
	if (!file_stream.is_open())
		return false;

	uint32_t size = file_stream.seekg(0, std::ifstream::end).tellg();
	file_stream.seekg(0, std::ifstream::beg);

	std::string buf(size + 1, '\0');
	file_stream.read((char*)buf.data(), size);

	int index = 0;
	do
	{
		index = buf.find('\t', index);
		if (index != std::string::npos)
		{
			buf.replace(index, 1, "");
			size--;
		}
		else
			break;

	} while (true);

	asset_profile_json_.Parse(buf.c_str(), size);
	if (!asset_profile_json_.IsObject())
		return false;

	JsonGetBoolMemberValue(asset_profile_json_, "virtual", assets_virtual_);

	rapidjson::Value *objects = nullptr;
	if (!JsonGetObjectMemberValue(asset_profile_json_, "objects", &objects))
	{
		DCHECK(0);
		return false;
	}

	bool ret = true;
	if (assets_virtual_)
	{
		for (auto obj = objects->MemberBegin(); obj != objects->MemberEnd(); obj++)
		{
			std::shared_ptr<mcfe::LegacyAssetFile> file = std::make_shared<mcfe::LegacyAssetFile>();
			std::string name(obj->name.GetString(), obj->name.GetStringLength());
			file->virtual_file_path = base_dir_ + L"\\assets\\virtual\\legacy\\" + std::wstring(name.begin(), name.end());
			if (!obj->value.IsObject())
			{
				ret = false;
				continue;
			}
				
			if(!JsonGetStringMemberValue(obj->value, "hash", file->sha1))
			{
				ret = false;
				continue;
			}
			if (!JsonGetUIntMemberValue(obj->value, "size", file->file_size))
			{
				ret = false;
				continue;
			}

			if(file->sha1.length() <2 || file->file_size == 0)
			{
				ret = false;
				continue;
			}

			std::wstring sha1(file->sha1.begin(), file->sha1.end());
			file->download_url = mcfe::g_official_assets_file_url_prefix + file->sha1.substr(0, 2) + "/" + file->sha1;
			file->file_path = base_dir_ + L"\\assets\\objects\\" + sha1.substr(0, 2) + L"\\";
			file->file_path += sha1;
			file->type = mcfe::kTypeAsset;
			file->flag |= FILE_FLAG_VIRTUAL_GAME_ASSETS;

			asset_files_.push_back(file);

		}
	}
	else
	{
		for (auto obj = objects->MemberBegin(); obj != objects->MemberEnd(); obj++)
		{
			std::shared_ptr<mcfe::GameFile> file = std::make_shared<mcfe::GameFile>();
			if (!obj->value.IsObject())
			{
				ret = false;
				continue;
			}
			if (!JsonGetStringMemberValue(obj->value, "hash", file->sha1))
			{
				ret = false;
				continue;
			}
			if (!JsonGetUIntMemberValue(obj->value, "size", file->file_size))
			{
				ret = false;
				continue;
			}

			if (file->sha1.length() < 2 || file->file_size == 0)
			{
				ret = false;
				continue;
			}

			std::wstring sha1(file->sha1.begin(), file->sha1.end());
			file->download_url = mcfe::g_official_assets_file_url_prefix + file->sha1.substr(0, 2) + "/" + file->sha1;
			file->file_path = base_dir_ + L"\\assets\\objects\\" + sha1.substr(0, 2) + L"\\";
			file->file_path += sha1;
			file->type = mcfe::kTypeAsset;

			asset_files_.push_back(file);

		}
	}
	
	return ret;
}

std::wstring MinecraftProfile::sid()
{
	wchar_t buffer[64] = { 0 };
	GetPrivateProfileStringW(L"info", L"sid", L"", buffer, 63, std::wstring(client_dir_ + L"\\craftbox.ini").c_str());
	return buffer;
}

