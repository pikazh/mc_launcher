#pragma once

#include <string>
#include <memory>

#include "base\NetworkTask.h"
#include "base\sigslot.h"

class NetworkTaskDispatcher;
class YggdrasilAuthenticate;
class YggdrasilRefresh;
class YggdrasilValidate;
class YggdrasilSignout;
class YggdrasilInvalidate;

class YggdrasilAuthentication
	: public sigslot::has_slots<>
	, public std::enable_shared_from_this<YggdrasilAuthentication>
{
public:

	static YggdrasilAuthentication* global();
	YggdrasilAuthentication();
	virtual ~YggdrasilAuthentication();


	enum AuthState
	{
		kAuthenticating,
		kAuthenticate_failed,
		kValidating,
		kValidate_failed,
		kRefreshing,
		kRefresh_failed,
		kSignouting,
		kInvalidating,
		kInvalidated,
		kValidated,
	};

public:
	sigslot::signal1<AuthState> authenticate_state_changed;

	void authenticate(const std::string &user_name, const std::string &password);
	void refresh(const std::string &access_token, const std::string &client_token);
	void validate(const std::string &access_token, const std::string &client_token);
	void signout(const std::string &user_name, const std::string &password);
	void invalidate(const std::string &access_token, const std::string &client_token);

	const std::string &user_guid() { return user_guid_; }
	const std::string &player_name() { return player_name_; }
	const std::string &client_token() { return client_token_; }
	const std::string &access_token() { return access_token_; }
	const std::string &email_addr() { return email_addr_; }
	const std::string &password() { return password_; }

	const std::wstring &err_message() { return err_message_; }
	
	AuthState authenticate_state() { return state_; }

protected:
	void emit_authenticate_state_changed_signal(AuthState state);

	void on_network_task_state_changed(std::shared_ptr<NetworkTask> task);
	void on_password_authenticate_finished(std::shared_ptr<YggdrasilAuthenticate> &task);
	void on_refresh_finished(std::shared_ptr<YggdrasilRefresh> &task);
	void on_validate_finished(std::shared_ptr<YggdrasilValidate> &task);
	void on_signout_finished(std::shared_ptr<YggdrasilSignout> &task);
	void on_invalidate_finished(std::shared_ptr<YggdrasilInvalidate> &task);
private:
	std::shared_ptr<NetworkTaskDispatcher> network_dispatch_;

	std::string access_token_;
	std::string client_token_;
	std::string user_guid_;
	std::string player_name_;
	std::string email_addr_;
	std::string password_;
	std::wstring err_message_;

	AuthState state_ = kInvalidated;
};

