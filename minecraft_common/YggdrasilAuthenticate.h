#pragma once

#include "YggdrasilRequest.h"

#include <memory>
#include <string>

class YggdrasilAuthenticate
	: public YggdrasilRequest
{
public:
	YggdrasilAuthenticate(std::shared_ptr<NetworkTaskDispatcher> task_dispatch);
	virtual ~YggdrasilAuthenticate();

	bool authenticate(const std::string &user_name, const std::string& password);

	bool get_response(std::wstring &err_message, std::string &access_token, std::string &client_token, std::string &user_guid, std::string &user_name);
};

