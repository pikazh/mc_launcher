#pragma once

#include "curl_easy.h"
#include "sigslot.h"

#include <memory>

class NetworkTask
	: public std::enable_shared_from_this<NetworkTask>
{
	friend class NetworkTaskDispatcher;

public:
	NetworkTask();
	virtual ~NetworkTask();

	enum State
	{
		kStop = 0,
		kWaiting,
		kRunning,
	};
public:
	curl::curl_easy& easy()
	{
		return easy_;
	}

	State state() { return state_; }

	virtual uint64_t dl_current() { return dl_curr_; }
	virtual uint64_t dl_total() { return dl_total_; }

	void set_cancel(bool cancel) { cancel_ = cancel; }
	bool is_cancelled() { return cancel_; }
	uint32_t task_id() { return task_id_; }

	sigslot::signal2<std::shared_ptr<NetworkTask>, State> state_changed;

protected:
	static int progress_callback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);
	static size_t header_callback(void *buffer, size_t size, size_t nitems, void *userdata);
	void set_state(State s) { state_ = s; }

	void set_curl_result(CURLcode res) { response_code_ = res; }
	CURLcode curl_result() { return response_code_; }

	std::string& response_header() { return response_header_; }

	std::string last_modified();

	//在被下载调度器调度之前调用。如果在被调度之前cancel了，则不会产生该回调
	//返回值：返回false的话，后续就不会被调度了，这种情况下也不会产生on_after_schedule回调
	virtual bool on_before_schedule();

	//当下载任务已经被调度，那么在调度完成之后（即使调用了cancel），会产生该回调
	virtual bool on_after_schedule();

	static uint32_t generate_task_id();

	virtual struct curl_slist *on_set_header(struct curl_slist* h);
private:
	curl::curl_easy easy_;
	bool cancel_ = false;
	State state_ = kStop;
	double dl_curr_ = 0;
	double dl_total_ = 0;
	struct curl_slist *headers_ = nullptr;

	CURLcode response_code_ = CURLE_OK;

	std::string response_header_;
	uint32_t task_id_ = NetworkTask::generate_task_id();
};

