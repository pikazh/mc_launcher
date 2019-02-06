#pragma once

#include "base/sigslot.h"
#include "Duilib/UIlib.h"
#include "UserJoinedServerInfo.h"

#include <string>


class CraftBoxUserJoinServerInfos
	: public sigslot::has_slots<>
{
public:

	static CraftBoxUserJoinServerInfos* instance();

	void update_craftbox_user_join_server_infos(std::wstring sid, std::wstring pid);
	void clear();
	bool is_fetch_finished();
	std::map<std::wstring, std::shared_ptr<JoinServerInfo>> & join_server_infos();

	void set_selected_sid(std::wstring sid);
	void set_selected_pid(std::wstring pid);

	std::wstring &select_sid();
	std::wstring &select_pid();

	sigslot::signal0<> fetch_finished;

protected:
	void on_fetch_user_join_server_info_finished();

private:
	std::map<std::wstring, std::shared_ptr<JoinServerInfo>> join_server_infos_;
	std::shared_ptr<UserJoinedServerInfo> user_join_server_req_;
	std::wstring select_pid_;
	std::wstring select_sid_;
	bool fetch_finish_ = false;
};


