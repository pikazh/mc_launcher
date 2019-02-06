#pragma once

#include <string>
#include <memory>

#include "MinecraftCommon.h"

class MinecraftUserIdentity
{
public:
	MinecraftUserIdentity() = default;
	virtual ~MinecraftUserIdentity() = default;

public:
	std::wstring &player_name() { return player_name_; }
	void set_player_name(const std::wstring &player_name) { player_name_ = player_name; }
	std::wstring &player_uuid() { return player_uuid_; }
	void set_player_uuid(const std::wstring &player_uuid) {	player_uuid_ = player_uuid; }
	std::wstring &access_token() { return access_token_; }
	void set_access_token(const std::wstring &access_token) { access_token_ = access_token; }
	std::wstring &user_type() { return user_type_; }
	void set_user_type(const std::wstring &user_type) { user_type_ = user_type; }
	std::wstring &user_properties() { return user_properties_; }
	void set_user_properties(const std::wstring &user_properties) { user_properties_ = user_properties; }

private:
	std::wstring player_name_;
	std::wstring player_uuid_;
	std::wstring access_token_;
	std::wstring user_type_;
	std::wstring user_properties_ = L"{}";
};
