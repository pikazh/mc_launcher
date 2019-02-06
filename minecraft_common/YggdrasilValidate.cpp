#include "YggdrasilValidate.h"
#include "document.h"
#include "writer.h"
#include "glog\logging.h"
#include "base\NetworkTaskDispatcher.h"
#include "base\JsonHelper.h"

YggdrasilValidate::YggdrasilValidate(std::shared_ptr<NetworkTaskDispatcher> task_dispatch)
	: YggdrasilRequest(task_dispatch)
{
	this->set_request_type(kValidate);
}


YggdrasilValidate::~YggdrasilValidate()
{
}

bool YggdrasilValidate::validate(const std::string &access_token, const std::string &client_token)
{
	access_token_ = access_token;
	client_token_ = client_token;

	rapidjson::Document doc;
	auto &alloca = doc.GetAllocator();
	rapidjson::Value root(rapidjson::kObjectType);
	root.AddMember("accessToken", rapidjson::Value(access_token.c_str(), access_token.length(), alloca), alloca);
	root.AddMember("clientToken", rapidjson::Value(client_token.c_str(), client_token.length(), alloca), alloca);

	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	if (root.Accept(writer))
	{
		easy().add<CURLOPT_URL>("https://authserver.mojang.com/validate");
		easy().add<CURLOPT_POSTFIELDSIZE>(buffer.GetSize());
		easy().add<CURLOPT_COPYPOSTFIELDS>(buffer.GetString());

		network_task_dispatcher()->add_task(this->shared_from_this());
		return true;
	}
	else
	{
		DCHECK(0);
		return false;
	}
}

bool YggdrasilValidate::get_response(std::wstring &err_message, std::string &access_token, std::string &client_token)
{
	if (this->recv_buffer().empty())
	{
		long http_code = 0;
		curl_easy_getinfo(easy().get_curl(), CURLINFO_RESPONSE_CODE, &http_code);
		if (http_code == 204)
		{
			access_token = access_token_;
			client_token = client_token_;
			return true;
		}
		else
		{
			err_message = L"未知错误";
			return false;
		}
	}
	else
	{
		rapidjson::Document doc;
		std::string err_msg;
		doc.Parse(this->recv_buffer().c_str(), this->recv_buffer().length());
		if (doc.IsObject())
		{
			JsonGetStringMemberValue(doc, "error", err_msg);
			if (err_msg == "ForbiddenOperationException")
			{
				err_message == L"不合法的令牌";
			}
			else
			{
				err_message = L"未知错误";
			}
		}
	}

	return false;
}
