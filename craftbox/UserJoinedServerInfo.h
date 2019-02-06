#pragma once

#include "base/HttpDownloadToBufferWithCache.h"
#include "base/sigslot.h"

#include <map>
#include <string>

struct PlayerInfoInServer
{
	std::wstring player_name;
	std::wstring pid;
	std::string ban;
};

struct JoinServerInfo
{
	std::wstring sid;
	std::wstring server_name;
	std::wstring host;
	uint16_t port = 25565;
	
	std::vector<std::shared_ptr<PlayerInfoInServer>> players;
};



class UserJoinedServerInfo
	: public sigslot::has_slots<>
	, public std::enable_shared_from_this<UserJoinedServerInfo>
{
public:
	UserJoinedServerInfo();
	virtual ~UserJoinedServerInfo();

public:
	void begin_fetch(const std::string &cookie);
	void stop_fetch();

	sigslot::signal0<> fetch_finished;

	auto &join_server_infos() 
	{
		return join_server_infos_;
	}

protected:
	void on_nettask_state_changed(std::shared_ptr<NetworkTask> task, NetworkTask::State state);

	void parse_buffer(const std::string &buf);
private:
	std::shared_ptr<HttpDownloadToBufferWithCache> http_download_;
	std::map<std::wstring, std::shared_ptr<JoinServerInfo>> join_server_infos_;
};
