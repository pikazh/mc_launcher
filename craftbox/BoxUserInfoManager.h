#pragma once

#include "base/sigslot.h"
#include "BoxUserInfoRequest.h"

#include <string>
#include <memory>

class BoxUserInfoManager
	: public std::enable_shared_from_this<BoxUserInfoManager>
	, public sigslot::has_slots<>
{
public:
	static BoxUserInfoManager* global();
	BoxUserInfoManager();
	virtual ~BoxUserInfoManager();

	struct BoxUserInfo
	{
		std::string user_name;
		std::string head_url;
	};


	void auth_with_cookie(const std::string &cookie);
	void logout();
	bool is_logined();

	std::shared_ptr<BoxUserInfo> user_info()
	{
		return user_info_;
	}

	std::string user_cookie()
	{
		return cookie_;
	}

	sigslot::signal0<> fetching_user_info;
	sigslot::signal1<bool> fetch_user_info_finished;
	sigslot::signal0<> logouted;

protected:
	void on_user_info_request_task_state_changed(std::shared_ptr<NetworkTask> task, NetworkTask::State state);

private:
	std::shared_ptr<BoxUserInfoRequest> user_info_req_obj_ = nullptr;
	std::shared_ptr<BoxUserInfo> user_info_;
	std::string cookie_;
};

