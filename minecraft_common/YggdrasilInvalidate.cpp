#include "YggdrasilInvalidate.h"
#include "document.h"
#include "writer.h"
#include "glog\logging.h"
#include "base\NetworkTaskDispatcher.h"
#include "base\JsonHelper.h"


YggdrasilInvalidate::YggdrasilInvalidate(std::shared_ptr<NetworkTaskDispatcher> task_dispatch)
	: YggdrasilRequest(task_dispatch)
{
	this->set_request_type(kInvalidate);
}


YggdrasilInvalidate::~YggdrasilInvalidate()
{
}

bool YggdrasilInvalidate::invalidate(const std::string &access_token, const std::string &client_token)
{
	rapidjson::Document doc;
	auto &alloca = doc.GetAllocator();
	rapidjson::Value root(rapidjson::kObjectType);
	root.AddMember("accessToken", rapidjson::Value(access_token.c_str(), access_token.length(), alloca), alloca);
	root.AddMember("clientToken", rapidjson::Value(client_token.c_str(), client_token.length(), alloca), alloca);

	rapidjson::StringBuffer  buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	if (root.Accept(writer))
	{
		easy().add<CURLOPT_URL>("https://authserver.mojang.com/invalidate");
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

bool YggdrasilInvalidate::get_response()
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
