#pragma once
#include "YggdrasilRequest.h"

class YggdrasilRefresh
	: public YggdrasilRequest
{
public:
	YggdrasilRefresh(std::shared_ptr<NetworkTaskDispatcher> task_dispatch);
	virtual ~YggdrasilRefresh();

	bool refresh(const std::string &access_token, const std::string &client_token);

	bool get_response(std::wstring &error, std::string &access_token, std::string &client_token, std::string &user_guid, std::string &user_name);
};

