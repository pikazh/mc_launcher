#pragma once

#include <memory>
#include <string>
#include <thread>
#include <mutex>
#include <vector>
#include <map>

#include "NetworkTask.h"
#include "curl/curl.h"
#include "curl_easy.h"
#include "curl_multi.h"
#include "sigslot.h"
#include "closure.h"
#include "asynchronous_task_dispatch.h"

class NetworkTaskDispatcher
	: public std::enable_shared_from_this<NetworkTaskDispatcher>
{
public:
	static NetworkTaskDispatcher* global();
	NetworkTaskDispatcher()
	{
		thread_task_dispatch_ = AsyncTaskDispatch::current();
	}
	virtual ~NetworkTaskDispatcher();

	void add_task(std::shared_ptr<NetworkTask> task);
	bool start_schedule();
	void stop_schedule();

	sigslot::signal1<const std::shared_ptr<NetworkTask>> task_state_changed;

protected:
	void schedule_function();
	bool handle_new_task(curl::curl_multi &multi);
	void handle_curl_messages(curl::curl_multi &multi);
	void handle_cancelled_task(curl::curl_multi &multi);

	void emit_network_task_start(std::shared_ptr<NetworkTask>& task);
	void emit_network_task_stop(std::shared_ptr<NetworkTask>& task);

private:
	bool running_ = false;
	std::thread schedule_thread_;

	std::recursive_mutex lock_;
	std::list<std::shared_ptr<NetworkTask>> need_schedule_tasks_;
	std::map<CURL*, std::shared_ptr<NetworkTask>> schedule_tasks_;
	std::shared_ptr<AsyncTaskDispatch> thread_task_dispatch_;
};

