#pragma once

#include "base/HttpDownloadToBufferWithCache.h"
#include "base/sigslot.h"

struct CraftBoxPlayerTokenInfo
{
	std::wstring pid;
	std::wstring player_name;
	std::string ban;
	std::wstring session;
	std::wstring err_msg;
};

class CraftBoxTokenFetcher
	: public sigslot::has_slots<>
	, public std::enable_shared_from_this<CraftBoxTokenFetcher>
{
public:
	CraftBoxTokenFetcher();
	virtual ~CraftBoxTokenFetcher();

	void begin_fetch(const std::string &cookie, const std::string &pid);
	void stop_fetch();

	sigslot::signal0<> fetch_finished;

	auto &player_token_info()
	{
		return craftbox_player_token_info_;
	}

protected:
	void on_nettask_state_changed(std::shared_ptr<NetworkTask> task, NetworkTask::State state);

	void parse_buffer(const std::string &buf);
private:
	std::shared_ptr<HttpDownloadToBufferWithCache> http_download_;
	std::shared_ptr<CraftBoxPlayerTokenInfo> craftbox_player_token_info_;
	std::wstring pid_;
};

