#include "YggdrasilRequest.h"
#include "glog\logging.h"

YggdrasilRequest::YggdrasilRequest(std::shared_ptr<NetworkTaskDispatcher> task_dispatch)
{
	this->task_dispatch_ = task_dispatch;
	easy().add<CURLOPT_POST>(1);
	easy().add<CURLOPT_FOLLOWLOCATION>(1);
	easy().add<CURLOPT_WRITEFUNCTION>(&YggdrasilRequest::write_callback);
	easy().add<CURLOPT_WRITEDATA>(this);
}


YggdrasilRequest::~YggdrasilRequest()
{
}

bool YggdrasilRequest::on_before_schedule()
{
	__super::on_before_schedule();
	recv_buff_.clear();
	return true;
}

struct curl_slist* YggdrasilRequest::on_set_header(struct curl_slist* h)
{
	h = curl_slist_append(h, "Content-Type: application/json");
	h = __super::on_set_header(h);
	return h;
}

size_t YggdrasilRequest::write_callback(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	YggdrasilRequest *pthis = (YggdrasilRequest*)userdata;
	auto sz = size*nmemb;
	if (pthis)
	{
		pthis->recv_buff_.append((char*)ptr, sz);
	}

	return sz;
}

NetworkTaskDispatcher* YggdrasilRequest::network_task_dispatcher()
{
	DCHECK(!!task_dispatch_);
	return task_dispatch_.get();
}

std::string& YggdrasilRequest::recv_buffer()
{
	return recv_buff_;
}
