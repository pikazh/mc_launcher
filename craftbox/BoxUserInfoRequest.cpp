#include "BoxUserInfoRequest.h"
#include "document.h"
#include "base/JsonHelper.h"
#include "base/at_exit_manager.h"

BoxUserInfoRequest::BoxUserInfoRequest()
{
}


BoxUserInfoRequest::~BoxUserInfoRequest()
{
}

void BoxUserInfoRequest::set_cookie(const std::string & cookie)
{
	cookie_ = cookie;
}

bool BoxUserInfoRequest::on_before_schedule()
{
	response_.clear();
	easy().add<CURLOPT_URL>("http://user.zuimc.com/Api/getprofile");
	easy().add<CURLOPT_COOKIE>(cookie_.c_str());
	easy().add<CURLOPT_WRITEFUNCTION>(&BoxUserInfoRequest::on_buffer_arrive);
	easy().add<CURLOPT_WRITEDATA>(this);

	__super::on_before_schedule();
	return true;
}

bool BoxUserInfoRequest::on_after_schedule()
{
	AtExitManager at_exit;
	at_exit.register_callback(make_closure([this]() {NetworkTask::on_after_schedule(); }));

	success_ = false;
	if (this->is_cancelled() || CURLE_OK != curl_result())
		return false;

	rapidjson::Document doc;
	doc.Parse(response_.c_str(), response_.length());
	if (doc.IsObject())
	{
		std::string code;
		JsonGetStringMemberValue(doc, "code", code);
		if (code.empty())
		{
			return false;
		}
		
		code_ = atoi(code.c_str());

		JsonGetStringMemberValue(doc, "msg", msg_);

		if (code_ == 0)
		{
			rapidjson::Value *val = nullptr;
			JsonGetObjectMemberValue(doc, "data", &val);
			if (val == nullptr)
			{
				return false;
			}
			 
			JsonGetStringMemberValue(*val, "nick", nick_name_);
			if (nick_name_.empty())
			{
				return false;
			}

			JsonGetStringMemberValue(*val, "avatar", header_url_);
		}

		success_ = true;
	}

	return true;
}

size_t BoxUserInfoRequest::on_buffer_arrive(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	auto sz = size*nmemb;
	BoxUserInfoRequest *pthis = (BoxUserInfoRequest*)userdata;
	if (pthis)
	{
		pthis->response_.append((char*)ptr, sz);
	}

	return sz;
}
