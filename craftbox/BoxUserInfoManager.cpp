#include "BoxUserInfoManager.h"
#include "base/asynchronous_task_dispatch.h"
#include "base/NetworkTaskDispatcher.h"
#include "glog/logging.h"

BoxUserInfoManager* BoxUserInfoManager::global()
{
	static std::shared_ptr<BoxUserInfoManager> obj = std::make_shared<BoxUserInfoManager>();
	return obj.get();
}

BoxUserInfoManager::BoxUserInfoManager()
{
}

BoxUserInfoManager::~BoxUserInfoManager()
{
}

void BoxUserInfoManager::auth_with_cookie(const std::string &cookie)
{
	cookie_ = cookie;
	user_info_req_obj_ = std::make_shared<BoxUserInfoRequest>();
	user_info_req_obj_->set_cookie(cookie);
	user_info_req_obj_->state_changed.connect(this, &BoxUserInfoManager::on_user_info_request_task_state_changed);
	NetworkTaskDispatcher::global()->add_task(user_info_req_obj_);
	fetching_user_info();
}

void BoxUserInfoManager::logout()
{
	if (!!user_info_req_obj_)
	{
		user_info_req_obj_->set_cancel(true);
		user_info_req_obj_.reset();
	}

	cookie_.clear();
	user_info_.reset();

	logouted();
}

void BoxUserInfoManager::on_user_info_request_task_state_changed(std::shared_ptr<NetworkTask> task, NetworkTask::State state)
{
	if (task->task_id() == user_info_req_obj_->task_id())
	{
		if (state == NetworkTask::kStop)
		{
			if (user_info_req_obj_->success())
			{
				if (user_info_req_obj_->code() == 0)
				{
					user_info_.reset(new BoxUserInfo);
					user_info_->user_name = user_info_req_obj_->nick_name();
					user_info_->head_url = user_info_req_obj_->header_url();

					fetch_user_info_finished(true);
					return;
				}
				else
				{
					LOG(ERROR) << "get user info failed. code: " << user_info_req_obj_->code() << " , msg: " << user_info_req_obj_->msg();
				}
			}

			fetch_user_info_finished(false);

			user_info_req_obj_.reset();
		}
	}
}

bool BoxUserInfoManager::is_logined()
{
	return !!user_info_;
}
