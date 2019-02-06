#include "YggdrasilAuthenticate.h"
#include "document.h"
#include "writer.h"
#include "glog\logging.h"
#include "base\NetworkTaskDispatcher.h"
#include "base\JsonHelper.h"

YggdrasilAuthenticate::YggdrasilAuthenticate(std::shared_ptr<NetworkTaskDispatcher> task_dispatch)
	: YggdrasilRequest(task_dispatch)
{
	this->set_request_type(kAuthenticate);
}


YggdrasilAuthenticate::~YggdrasilAuthenticate()
{
}

bool YggdrasilAuthenticate::authenticate(const std::string &user_name, const std::string& password)
{
	rapidjson::Document doc;
	auto &alloca = doc.GetAllocator();
	rapidjson::Value root(rapidjson::kObjectType);
	rapidjson::Value agent(rapidjson::kObjectType);
	agent.AddMember("name", "Minecraft", alloca);
	agent.AddMember("version", 1, alloca);
	root.AddMember("agent", agent, alloca);
	root.AddMember("username", rapidjson::Value(user_name.c_str(), user_name.length(), alloca), alloca);
	root.AddMember("password", rapidjson::Value(password.c_str(), password.length(), alloca), alloca);

	rapidjson::StringBuffer  buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	if(root.Accept(writer))
	{
		easy().add<CURLOPT_URL>("https://authserver.mojang.com/authenticate");
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

bool YggdrasilAuthenticate::get_response(std::wstring &err_message, std::string &access_token, std::string &client_token, std::string &user_guid, std::string &user_name)
{
	auto curl_ret = curl_result();
	if (CURLE_OK != curl_ret)
	{
		if (CURLE_OPERATION_TIMEDOUT == curl_ret)
		{
			err_message = L"连接官方认证服务器超时";
		}
		else
		{
			err_message = L"与官方认证服务器通讯的过程发生错误";
		}

		return false;
	}
	rapidjson::Document doc;
	auto &buf = this->recv_buffer();
	doc.Parse(buf.c_str(), buf.length());
	if (doc.IsObject())
	{
		std::string error;
		if (JsonGetStringMemberValue(doc, "errorMessage", error))
		{
			auto index = error.find("Invalid credentials.");
			if (index == 0)
			{
				if (error.length() == strlen("Invalid credentials."))
				{
					err_message = L"你登录的太频繁了，请过段时间再登陆";
				}
				else
				{
					err_message = L"不正确的用户名或者密码";
				}
			}
			else
			{
				DCHECK(0);
				err_message = L"未知错误";
			}

			return false;
		}
		else
		{
			rapidjson::Value *profile = nullptr;

			if (JsonGetStringMemberValue(doc, "accessToken", access_token)
				&& JsonGetStringMemberValue(doc, "clientToken", client_token)
				)
			{
				if (JsonGetObjectMemberValue(doc, "selectedProfile", &profile)
					&& JsonGetStringMemberValue(*profile, "id", user_guid)
					&& JsonGetStringMemberValue(*profile, "name", user_name)
					)
					return true;
				else
				{
					err_message = L"无法获取玩家信息";
					return false;
				}
			}
			else
			{
				DCHECK(0);
				err_message = L"不支持的通讯格式";

				return false;
			}
		}
	}
	else
	{
		DCHECK(0);
		err_message = L"未知错误";
	}

	return false;
}
