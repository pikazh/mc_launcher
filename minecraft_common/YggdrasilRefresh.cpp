#include "YggdrasilRefresh.h"
#include "document.h"
#include "writer.h"
#include "glog\logging.h"
#include "base\NetworkTaskDispatcher.h"
#include "base\JsonHelper.h"

YggdrasilRefresh::YggdrasilRefresh(std::shared_ptr<NetworkTaskDispatcher> task_dispatch)
	: YggdrasilRequest(task_dispatch)
{
	this->set_request_type(kRefresh);
}


YggdrasilRefresh::~YggdrasilRefresh()
{
}

bool YggdrasilRefresh::refresh(const std::string &access_token, const std::string &client_token)
{
	rapidjson::Document doc;
	auto &alloca = doc.GetAllocator();
	rapidjson::Value root(rapidjson::kObjectType);
	root.AddMember("accessToken", rapidjson::Value(access_token.c_str(), access_token.length(), alloca), alloca);
	root.AddMember("clientToken", rapidjson::Value(client_token.c_str(), client_token.length(), alloca), alloca);

	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	if (root.Accept(writer))
	{
		easy().add<CURLOPT_URL>("https://authserver.mojang.com/refresh");
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

bool YggdrasilRefresh::get_response(std::wstring &error, std::string &access_token, std::string &client_token, std::string &user_guid, std::string &user_name)
{
	rapidjson::Document doc;
	auto &buf = this->recv_buffer();
	doc.Parse(buf.c_str(), buf.length());
	if (doc.IsObject())
	{
		std::string err_msg;
		if (JsonGetStringMemberValue(doc, "error", err_msg))
		{
			if (err_msg == "ForbiddenOperationException")
			{
				error = L"不合法的令牌";
			}
			else
			{
				error = L"未知错误";
			}
			return false;
		}

		error = L"通讯数据格式错误";

		if (!JsonGetStringMemberValue(doc, "accessToken", access_token))
			return false;

		if (!JsonGetStringMemberValue(doc, "clientToken", client_token))
			return false;

		rapidjson::Value *profile = nullptr;
		if (!JsonGetObjectMemberValue(doc, "selectedProfile", &profile))
			return false;

		if (!JsonGetStringMemberValue(*profile, "id", user_guid))
			return false;

		if (!JsonGetStringMemberValue(*profile, "name", user_name))
			return false;

		error.clear();
		return true;
	}
	else
	{
		error = L"通讯数据格式错误";
	}
	return false;
}
