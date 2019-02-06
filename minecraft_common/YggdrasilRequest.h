#pragma once
#include "base\NetworkTask.h"

class YggdrasilRequest
	: public NetworkTask
{
public:
	YggdrasilRequest(std::shared_ptr<NetworkTaskDispatcher> task_dispatch);
	virtual ~YggdrasilRequest();

	enum RequestType
	{
		kUnknown,
		kAuthenticate,
		kRefresh,
		kValidate,
		kSignout,
		kInvalidate,
	};

	RequestType request_type()
	{
		return type_;
	}

protected:
	virtual bool on_before_schedule() override;
	virtual struct curl_slist* on_set_header(struct curl_slist* h) override;

	static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata);

	NetworkTaskDispatcher* network_task_dispatcher();

	std::string& recv_buffer();
	void set_request_type(RequestType type) { type_ = type; }
private:
	std::shared_ptr<NetworkTaskDispatcher> task_dispatch_;
	std::string recv_buff_;

	RequestType type_ = kUnknown;
};

