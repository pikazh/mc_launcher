#include "CraftBoxUserJoinServerInfos.h"
#include "BoxUserInfoManager.h"

#include <memory>


CraftBoxUserJoinServerInfos* CraftBoxUserJoinServerInfos::instance()
{
	static std::shared_ptr<CraftBoxUserJoinServerInfos> obj = std::make_shared<CraftBoxUserJoinServerInfos>();
	return obj.get();
}

void CraftBoxUserJoinServerInfos::update_craftbox_user_join_server_infos(std::wstring sid, std::wstring pid)
{
	select_pid_ = pid;
	select_sid_ = sid;
	user_join_server_req_.reset(new UserJoinedServerInfo);
	user_join_server_req_->fetch_finished.connect(this, &CraftBoxUserJoinServerInfos::on_fetch_user_join_server_info_finished);
	user_join_server_req_->begin_fetch(BoxUserInfoManager::global()->user_cookie());
}

void CraftBoxUserJoinServerInfos::clear()
{
	join_server_infos_.clear();
	fetch_finish_ = false;
}

bool CraftBoxUserJoinServerInfos::is_fetch_finished()
{
	return fetch_finish_;
}

std::map<std::wstring, std::shared_ptr<JoinServerInfo>> & CraftBoxUserJoinServerInfos::join_server_infos()
{
	return join_server_infos_;
}

void CraftBoxUserJoinServerInfos::set_selected_sid(std::wstring sid)
{
	select_sid_ = sid;
}

void CraftBoxUserJoinServerInfos::set_selected_pid(std::wstring pid)
{
	select_pid_ = pid;
}

std::wstring & CraftBoxUserJoinServerInfos::select_sid()
{
	return select_sid_;
}

std::wstring & CraftBoxUserJoinServerInfos::select_pid()
{
	return select_pid_;
}

void CraftBoxUserJoinServerInfos::on_fetch_user_join_server_info_finished()
{
	join_server_infos_ = user_join_server_req_->join_server_infos();
	user_join_server_req_.reset();
	fetch_finish_ = true;
	fetch_finished();
}

