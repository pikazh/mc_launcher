#include "MinecraftUserIdentityFactory.h"
#include "minecraft_common\YggdrasilAuthentication.h"
#include "glog/logging.h"
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include "md5.h"
#include <Strsafe.h>


std::shared_ptr<MinecraftUserIdentity> MinecraftUserIdentityFactory::create_user_identity_object(mcfe::UserAccountType type)
{
	std::shared_ptr<MinecraftUserIdentity> user_identity;
	switch (type)
	{
	case mcfe::kOffline:
	{
		auto identity = new MinecraftUserIdentity;
		std::wstring offline_player_name = AppConfig::global()->get_value_for_key(L"offline_player_name");
		identity->set_player_name(offline_player_name);
		identity->set_user_type(L"offline");
		identity->set_player_uuid(mcfe::offline_user_uuid(offline_player_name));

		{
			DWORD startMSCount = timeGetTime();
			time_t CurSysTime, BootSysTime;
			time(&CurSysTime);
			//将开机到现在的毫秒数转换为秒数，再用当前的时间减去，获得开机时间
			BootSysTime = CurSysTime - startMSCount / 1000;

			{
				byte md5_byte[16] = { 0 };
				CryptoPP::Weak::MD5 md5;
				std::string start_machine_time = std::to_string(BootSysTime);
				md5.Update((const byte*)start_machine_time.c_str(), start_machine_time.length());
				md5.Final(md5_byte);

				wchar_t md5_buf[33] = { 0 };
				for (auto i = 0; i < 16; i++)
				{
					StringCchPrintfW(md5_buf + i * 2, 3, L"%02x", md5_byte[i]);
				}

				identity->set_access_token(md5_buf);
			}
		}

		user_identity.reset(identity);
		break;
	}
		
	case mcfe::kOfficial:
	{
		auto yggdrasil_auth = YggdrasilAuthentication::global();
		auto identity = new MinecraftUserIdentity;
		identity->set_player_name(std::wstring(yggdrasil_auth->player_name().begin(), yggdrasil_auth->player_name().end()));
		identity->set_player_uuid(std::wstring(yggdrasil_auth->user_guid().begin(), yggdrasil_auth->user_guid().end()));
		identity->set_user_type(L"legacy");
		identity->set_access_token(std::wstring(yggdrasil_auth->access_token().begin(), yggdrasil_auth->access_token().end()));
		user_identity.reset(identity);
		break;
	}
	default:
		DCHECK(0);
		break;
	}

	return user_identity;
}

std::shared_ptr<MinecraftUserIdentity> MinecraftUserIdentityFactory::create_user_identity_object_with_player_name(mcfe::UserAccountType type, std::wstring user_name)
{
	std::shared_ptr<MinecraftUserIdentity> user_identity;
	switch (type)
	{
	case mcfe::kOffline:
	{
		auto identity = new MinecraftUserIdentity;
		identity->set_player_name(user_name);
		identity->set_user_type(L"offline");
		identity->set_player_uuid(mcfe::offline_user_uuid(user_name));

		{
			DWORD startMSCount = timeGetTime();
			time_t CurSysTime, BootSysTime;
			time(&CurSysTime);
			//将开机到现在的毫秒数转换为秒数，再用当前的时间减去，获得开机时间
			BootSysTime = CurSysTime - startMSCount / 1000;

			{
				byte md5_byte[16] = { 0 };
				CryptoPP::Weak::MD5 md5;
				std::string start_machine_time = std::to_string(BootSysTime);
				md5.Update((const byte*)start_machine_time.c_str(), start_machine_time.length());
				md5.Final(md5_byte);

				wchar_t md5_buf[33] = { 0 };
				for (auto i = 0; i < 16; i++)
				{
					StringCchPrintfW(md5_buf + i * 2, 3, L"%02x", md5_byte[i]);
				}

				identity->set_access_token(md5_buf);
			}
		}

		user_identity.reset(identity);
		break;
	}

	case mcfe::kOfficial:
	{
		auto yggdrasil_auth = YggdrasilAuthentication::global();
		auto identity = new MinecraftUserIdentity;
		identity->set_player_name(std::wstring(yggdrasil_auth->player_name().begin(), yggdrasil_auth->player_name().end()));
		identity->set_player_uuid(std::wstring(yggdrasil_auth->user_guid().begin(), yggdrasil_auth->user_guid().end()));
		identity->set_user_type(L"legacy");
		identity->set_access_token(std::wstring(yggdrasil_auth->access_token().begin(), yggdrasil_auth->access_token().end()));
		user_identity.reset(identity);
		break;
	}
	case mcfe::kCraftBox:
	{
		auto identity = new MinecraftUserIdentity;
		identity->set_player_name(user_name);
		identity->set_user_type(L"craftbox");
		identity->set_player_uuid(mcfe::offline_user_uuid(user_name));

		{
			DWORD startMSCount = timeGetTime();
			time_t CurSysTime, BootSysTime;
			time(&CurSysTime);
			//将开机到现在的毫秒数转换为秒数，再用当前的时间减去，获得开机时间
			BootSysTime = CurSysTime - startMSCount / 1000;

			{
				byte md5_byte[16] = { 0 };
				CryptoPP::Weak::MD5 md5;
				std::string start_machine_time = std::to_string(BootSysTime);
				md5.Update((const byte*)start_machine_time.c_str(), start_machine_time.length());
				md5.Final(md5_byte);

				wchar_t md5_buf[33] = { 0 };
				for (auto i = 0; i < 16; i++)
				{
					StringCchPrintfW(md5_buf + i * 2, 3, L"%02x", md5_byte[i]);
				}

				identity->set_access_token(md5_buf);
			}
		}

		user_identity.reset(identity);
		break;
	}
	}

	return user_identity;
}
