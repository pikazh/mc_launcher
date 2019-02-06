#pragma once
#include "YggdrasilRequest.h"

class YggdrasilValidate
	: public YggdrasilRequest
{
public:
	YggdrasilValidate(std::shared_ptr<NetworkTaskDispatcher> task_dispatch);
	virtual ~YggdrasilValidate();

	bool validate(const std::string &access_token, const std::string &client_token);

	bool get_response(std::wstring &err_message, std::string &access_token, std::string &client_token);

private:
	std::string access_token_;
	std::string client_token_;
};

