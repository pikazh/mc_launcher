#pragma once

#include "YggdrasilRequest.h"

class YggdrasilSignout
	: public YggdrasilRequest
{
public:
	YggdrasilSignout(std::shared_ptr<NetworkTaskDispatcher> task_dispatch);
	virtual ~YggdrasilSignout();

	bool signout(const std::string &user_name, const std::string &password);
	bool get_response();
};

