#pragma once

#include "base/HttpDownloadToBufferWithCache.h"
#include <memory>

class DefaultServerList
	: public sigslot::has_slots<>
	, public std::enable_shared_from_this<DefaultServerList>
{
public:
	struct ServerItem
	{
		std::string name;
		std::string host;
	};

	static DefaultServerList *instance();
	void fetch_server_list();
	void stop_fetch();

	bool is_fetch_finished();

	sigslot::signal0<> fetch_finished;

	std::vector<struct ServerItem> server_list();

protected:
	void on_nettask_state_changed(std::shared_ptr<NetworkTask> task, NetworkTask::State state);
	void parse_buffer(const std::string &buf);
private:
	std::shared_ptr<HttpDownloadToBufferWithCache> http_download_;



	std::vector<struct ServerItem> server_list_;
	bool fetch_finished_ = false;
};

