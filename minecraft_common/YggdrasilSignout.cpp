#include "YggdrasilSignout.h"
#include "document.h"
#include "writer.h"
#include "glog\logging.h"
#include "base\NetworkTaskDispatcher.h"
#include "base\JsonHelper.h"


YggdrasilSignout::YggdrasilSignout(std::shared_ptr<NetworkTaskDispatcher> task_dispatch)
	: YggdrasilRequest(task_dispatch)
{
	this->set_request_type(kSignout);
}

YggdrasilSignout::~YggdrasilSignout()
{
}

bool YggdrasilSignout::signout(const std::string &user_name, const std::string &password)
{
	rapidjson::Document doc;
	auto &alloca = doc.GetAllocator();
	rapidjson::Value root(rapidjson::kObjectType);
	root.AddMember("username", rapidjson::Value(user_name.c_str(), user_name.length(), alloca), alloca);
	root.AddMember("password", rapidjson::Value(password.c_str(), password.length(), alloca), alloca);

	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	if (root.Accept(writer))
	{
		easy().add<CURLOPT_URL>("https://authserver.mojang.com/signout");
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

bool YggdrasilSignout::get_response()
{
	if (this->recv_buffer().empty())
	{
		long http_code = 0;
		curl_easy_getinfo(easy().get_curl(), CURLINFO_RESPONSE_CODE, &http_code);
		if (http_code == 204)
			return true;
	}

	return false;
}
