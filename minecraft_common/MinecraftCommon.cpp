#include "MinecraftCommon.h"
#include "glog/logging.h"

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include "md5.h"

#include <codecvt>
#include <Strsafe.h>

namespace mcfe
{
	wchar_t *g_app_name = L"创世神启动器";

	char *g_official_minecraft_versions_json_url = "https://launchermeta.mojang.com/mc/game/version_manifest.json";
	char *g_integration_minecraft_version_json_url = "http://launcher.zuimc.com/package.php";

	char *g_java_info_url = "http://launcher.zuimc.com/download_java.php";
	char *g_outer_link_url = "http://www.zuimc.com";
	char *g_feedback_url = "https://jinshuju.net/f/cUynVG";
	char *g_update_info_url = "http://launcher.zuimc.com/update.php";

	char *g_official_libraries_file_url_prefix = "https://libraries.minecraft.net/";
	char *g_official_assets_file_url_prefix = "http://resources.download.minecraft.net/";
	wchar_t *g_server_list_page = L"http://user.zuimc.com/serverlist/old/1";
	wchar_t *g_main_page = L"http://launcher.zuimc.com/news.php";
	wchar_t *g_user_page = L"http://user.zuimc.com";

	std::wstring offline_user_uuid(const std::wstring & offline_player_name)
	{
		std::wstring uuid_gen_key = L"OfflinePlayer:" + offline_player_name;
		byte md5_byte[16] = { 0 };
		CryptoPP::Weak::MD5 md5;
		std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
		std::string uuid_gen_key_utf8 = converter.to_bytes(uuid_gen_key);
		md5.Update((const byte*)uuid_gen_key_utf8.c_str(), uuid_gen_key_utf8.length());
		md5.Final(md5_byte);

		wchar_t md5_buf[33] = { 0 };
		for (auto i = 0; i < 16; i++)
		{
			StringCchPrintfW(md5_buf + i * 2, 3, L"%02x", md5_byte[i]);
		}

		return md5_buf;
	}


	mcfe::MinecraftVersionCompareResult version_compare(const std::wstring &ver, const std::wstring &compare_to)
	{
		if (ver.empty() || compare_to.empty())
			return kInvalidVersion;

		int main_ver = 0;
		int middle_ver = 0;
		int little_ver = 0;

		int main_ver_compare_to = 0;
		int middle_ver_compare_to = 0;
		int little_ver_compare_to = 0;

		{
			auto index1 = ver.find(L".");
			if (index1 != 1)
				return kInvalidVersion;

			main_ver = ver[0] - L'0';
			if (main_ver != 1)
				return kInvalidVersion;

			auto index2 = ver.find(L".", index1 + 1);
			if (index2 == index1+1)
				return kInvalidVersion;
			else if(index2 == std::wstring::npos)
				middle_ver = _wtoi(ver.substr(index1 + 1).c_str());
			else
			{
				middle_ver = _wtoi(ver.substr(index1 + 1, index2-index1-1).c_str());
				little_ver = _wtoi(ver.substr(index2 + 1).c_str());
			}
		}
		
		{
			auto index1 = compare_to.find(L".");
			if (index1 != 1)
				return kInvalidVersion;

			main_ver_compare_to = compare_to[0] - L'0';
			if (main_ver_compare_to != 1)
				return kInvalidVersion;

			auto index2 = compare_to.find(L".", index1 + 1);
			if (index2 == index1 + 1)
				return kInvalidVersion;
			else if (index2 == std::wstring::npos)
				middle_ver_compare_to = _wtoi(compare_to.substr(index1 + 1).c_str());
			else
			{
				middle_ver_compare_to = _wtoi(compare_to.substr(index1 + 1, index2 - index1 - 1).c_str());
				little_ver_compare_to = _wtoi(compare_to.substr(index2 + 1).c_str());
			}
		}

		if (middle_ver != middle_ver_compare_to)
			return kMiddleVersionNotEqual;

		if (little_ver == little_ver_compare_to)
			return kVersionEqual;
		else if (little_ver < little_ver_compare_to)
			return kLittleVersionLess;
		else
			return kLittleVersionMore;
	}

}
