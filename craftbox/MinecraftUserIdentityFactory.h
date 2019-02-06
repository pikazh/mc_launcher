#pragma once

#include <string>
#include <memory>

#include "minecraft_common/MinecraftCommon.h"
#include "minecraft_common/MinecraftUserIdentity.h"
#include "AppConfig.h"

class MinecraftUserIdentityFactory
{
public:
	static std::shared_ptr<MinecraftUserIdentity> create_user_identity_object(mcfe::UserAccountType type);
	static std::shared_ptr<MinecraftUserIdentity> create_user_identity_object_with_player_name(mcfe::UserAccountType type, std::wstring user_name);
};
