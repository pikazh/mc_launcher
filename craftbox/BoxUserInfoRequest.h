#pragma once

#include "base/NetworkTask.h"

#include <string>

class BoxUserInfoRequest
	: public NetworkTask
{
public:
	BoxUserInfoRequest();
	virtual ~BoxUserInfoRequest();

	void set_cookie(const std::string &cookie);
public:
	virtual bool on_before_schedule() override;
	virtual bool on_after_schedule() override;
	bool success() { return success_; }
	int code() { return code_; }
	std::string &msg() { return msg_; }
	const std::string & nick_name() { return nick_name_; }
	const std::string & header_url() { return header_url_; }

protected:
	static size_t on_buffer_arrive(void *ptr, size_t size, size_t nmemb, void *userdata);
private:
	std::string cookie_;
	std::string response_;
	std::string nick_name_;
	std::string header_url_;
	std::string msg_;
	bool success_ = false;
	int code_ = 0;
};

