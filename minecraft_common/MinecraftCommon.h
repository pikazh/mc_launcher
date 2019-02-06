#pragma once

#include <string>
#include <list>

#define FILE_FLAG_NATIVE_LIBRARY 1
#define FILE_FLAG_VIRTUAL_GAME_ASSETS (1 << 1)

namespace mcfe
{
	enum UserAccountType
	{
		kOffline = 0,
		kCraftBox,
		kOfficial,
	};

	enum MinecraftGameFileType
	{
		kTypeUnknown = 0,
		kTypeClient,
		kTypeLibrary,
		kTypeAsset,
		kTypeProfile,
	};

	enum MinecraftVersionCompareResult
	{
		kInvalidVersion = 0,
		kMiddleVersionNotEqual,
		kLittleVersionLess,
		kLittleVersionMore,
		kVersionEqual,
	};

	struct GameFile
	{
		MinecraftGameFileType type = kTypeUnknown;
		std::wstring file_path;
		std::string download_url;
		unsigned int file_size = 0;
		std::string sha1;
		uint32_t flag = 0;
	};

	struct LegacyAssetFile : public GameFile
	{
		std::wstring virtual_file_path;
	};

	std::wstring offline_user_uuid(const std::wstring &offline_player_name);

	MinecraftVersionCompareResult version_compare(const std::wstring &ver, const std::wstring &compare_to);

	extern char *g_official_minecraft_versions_json_url;

	extern char *g_java_info_url;
	extern char *g_outer_link_url;
	extern char *g_feedback_url;
	extern char *g_update_info_url;
	extern wchar_t *g_app_name;

	extern char *g_official_libraries_file_url_prefix;
	extern char *g_official_assets_file_url_prefix;

	extern char *g_integration_minecraft_version_json_url;

	extern wchar_t *g_main_page;
	extern wchar_t *g_server_list_page;
	extern wchar_t *g_user_page;
};

