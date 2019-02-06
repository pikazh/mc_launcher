#pragma once
#include "YggdrasilRequest.h"

class YggdrasilInvalidate
	: public YggdrasilRequest
{
public:
	YggdrasilInvalidate(std::shared_ptr<NetworkTaskDispatcher> task_dispatch);
	virtual ~YggdrasilInvalidate();

	bool invalidate(const std::string &access_token, const std::string &client_token);
	bool get_response();
};

