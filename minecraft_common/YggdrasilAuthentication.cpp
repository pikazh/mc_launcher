#include "YggdrasilAuthentication.h"
#include "YggdrasilAuthenticate.h"
#include "YggdrasilRefresh.h"
#include "YggdrasilValidate.h"
#include "YggdrasilSignout.h"
#include "YggdrasilInvalidate.h"
#include "base\NetworkTaskDispatcher.h"
#include "glog\logging.h"

#include <memory>

YggdrasilAuthentication * YggdrasilAuthentication::global()
{
	static std::shared_ptr<YggdrasilAuthentication> s_global_obj = std::make_shared<YggdrasilAuthentication>();
	return s_global_obj.get();
}

YggdrasilAuthentication::YggdrasilAuthentication()
{
	network_dispatch_ = std::make_shared<NetworkTaskDispatcher>();
	network_dispatch_->task_state_changed.connect(this, &YggdrasilAuthentication::on_network_task_state_changed);
	bool ret = network_dispatch_->start_schedule();
	CHECK(ret);
}


YggdrasilAuthentication::~YggdrasilAuthentication()
{
	network_dispatch_->stop_schedule();
}

void YggdrasilAuthentication::authenticate(const std::string &user_name, const std::string &password)
{
	std::shared_ptr<YggdrasilAuthenticate> auth = std::make_shared<YggdrasilAuthenticate>(network_dispatch_);
	auth->authenticate(user_name, password);
	email_addr_ = user_name;
	password_ = password;
	emit_authenticate_state_changed_signal(kAuthenticating);
}

void YggdrasilAuthentication::refresh(const std::string &access_token, const std::string &client_token)
{
	std::shared_ptr<YggdrasilRefresh> task = std::make_shared<YggdrasilRefresh>(network_dispatch_);
	task->refresh(access_token, client_token);

	emit_authenticate_state_changed_signal(kRefreshing);
}

void YggdrasilAuthentication::validate(const std::string &access_token, const std::string &client_token)
{
	access_token_ = access_token;
	client_token_ = client_token;
	std::shared_ptr<YggdrasilValidate> task = std::make_shared<YggdrasilValidate>(network_dispatch_);
	task->validate(access_token, client_token);

	emit_authenticate_state_changed_signal(kValidating);
}

void YggdrasilAuthentication::signout(const std::string &user_name, const std::string &password)
{
	std::shared_ptr<YggdrasilSignout> task = std::make_shared<YggdrasilSignout>(network_dispatch_);
	task->signout(user_name, password);

	emit_authenticate_state_changed_signal(kSignouting);
}

void YggdrasilAuthentication::invalidate(const std::string &access_token, const std::string &client_token)
{
	std::shared_ptr<YggdrasilInvalidate> task = std::make_shared<YggdrasilInvalidate>(network_dispatch_);
	task->invalidate(access_token, client_token);
	emit_authenticate_state_changed_signal(kInvalidating);
}

void YggdrasilAuthentication::on_network_task_state_changed(std::shared_ptr<NetworkTask> task)
{
	if (task->state() != NetworkTask::kStop)
		return;

	std::shared_ptr<YggdrasilRequest> t = std::dynamic_pointer_cast<YggdrasilRequest>(task);
	if (!t)
	{
		return;
	}

	switch(t->request_type())
	{
	case YggdrasilRequest::kAuthenticate:
	{
		std::shared_ptr<YggdrasilAuthenticate> auth = std::dynamic_pointer_cast<YggdrasilAuthenticate>(task);
		DCHECK(!!auth);
		on_password_authenticate_finished(auth);
		break;
	}

	case YggdrasilRequest::kRefresh:
	{
		auto refresh_request = std::dynamic_pointer_cast<YggdrasilRefresh>(task);
		DCHECK(!!refresh_request);
		on_refresh_finished(refresh_request);
		break;
	}

	case YggdrasilRequest::kValidate:
	{
		auto validate_request = std::dynamic_pointer_cast<YggdrasilValidate>(task);
		DCHECK(!!validate_request);
		on_validate_finished(validate_request);
		break;
	}

	case YggdrasilRequest::kSignout:
	{
		auto req = std::dynamic_pointer_cast<YggdrasilSignout>(task);
		DCHECK(!!req);
		on_signout_finished(req);
		break;
	}

	case YggdrasilRequest::kInvalidate:
	{
		auto req = std::dynamic_pointer_cast<YggdrasilInvalidate>(task);
		DCHECK(!!req);
		on_invalidate_finished(req);
		break;
	}

	}
}

void YggdrasilAuthentication::on_password_authenticate_finished(std::shared_ptr<YggdrasilAuthenticate> &task)
{
	if (!task->get_response(err_message_, access_token_, client_token_, user_guid_, player_name_))
	{
		access_token_.clear();
		client_token_.clear();
		player_name_.clear();
		user_guid_.clear();

		emit_authenticate_state_changed_signal(kAuthenticate_failed);
	}
	else
	{
		emit_authenticate_state_changed_signal(kValidated);
	}
}

void YggdrasilAuthentication::emit_authenticate_state_changed_signal(AuthState state)
{
	std::weak_ptr<YggdrasilAuthentication> weak_obj = this->shared_from_this();
	AsyncTaskDispatch::current()->post_task(make_closure([weak_obj, state]()
	{
		auto obj = weak_obj.lock();
		if (!!obj)
		{
			obj->state_ = state;
			obj->authenticate_state_changed(obj->state_);
		}
	}), true);
}

void YggdrasilAuthentication::on_refresh_finished(std::shared_ptr<YggdrasilRefresh> &task)
{
	if (task->get_response(err_message_, access_token_, client_token_, user_guid_, player_name_))
	{
		emit_authenticate_state_changed_signal(kValidated);
	}
	else
	{
		emit_authenticate_state_changed_signal(kRefresh_failed);
	}
}

void YggdrasilAuthentication::on_validate_finished(std::shared_ptr<YggdrasilValidate> &task)
{
	if (task->get_response(err_message_, access_token_, client_token_))
	{
		emit_authenticate_state_changed_signal(kValidated);
	}
	else
	{
		emit_authenticate_state_changed_signal(kValidate_failed);
	}
}

void YggdrasilAuthentication::on_signout_finished(std::shared_ptr<YggdrasilSignout> &task)
{
	DCHECK(task->get_response());
	emit_authenticate_state_changed_signal(kInvalidated);
}

void YggdrasilAuthentication::on_invalidate_finished(std::shared_ptr<YggdrasilInvalidate> &task)
{
	DCHECK(task->get_response());
	emit_authenticate_state_changed_signal(kInvalidated);
}

