#include "NetworkTaskDispatcher.h"
#include "glog/logging.h"
#include "StringHelper.h"
#include "asynchronous_task_dispatch.h"
#include "sha.h"
#include "cryptlib.h"
#include "hex.h"
#include "files.h"

#include <algorithm>
#include <locale>
#include <Windows.h>
#include <shlwapi.h>
#include <shlobj.h>

#define MAX_DL_TASK_NUM (5)

NetworkTaskDispatcher::~NetworkTaskDispatcher()
{
}

bool NetworkTaskDispatcher::start_schedule()
{
	if (running_)
		return true;
	
	try
	{
		running_ = true;
		std::thread t(&NetworkTaskDispatcher::schedule_function, this);
		schedule_thread_ = std::move(t);
	}
	catch (std::exception &e)
	{
		LOG(ERROR) << "run download manager FAILED, err:" << e.what();

		running_ = false;
		if(schedule_thread_.joinable())
			schedule_thread_.detach();
		
		return false;
	}

	return true;
}

void NetworkTaskDispatcher::stop_schedule()
{
	running_ = false;
	if (schedule_thread_.joinable())
		schedule_thread_.join();
}

void NetworkTaskDispatcher::add_task(std::shared_ptr<NetworkTask> dl_task)
{
	auto obj = this->shared_from_this();
	thread_task_dispatch_->post_task(make_closure([obj, dl_task]()
	{
		dl_task->set_state(NetworkTask::kWaiting);
		dl_task->state_changed(dl_task, NetworkTask::kWaiting);
		obj->task_state_changed(dl_task);
	}), true);
	std::lock_guard<std::recursive_mutex> guard(lock_);
	need_schedule_tasks_.push_back(dl_task);
}

void NetworkTaskDispatcher::schedule_function()
{
	curl::curl_multi multi;

	try
	{
		while (running_)
		{
			::Sleep(100);

			if (handle_new_task(multi))
			{
				while (!multi.perform());
				handle_curl_messages(multi);
			}

			while (running_ && multi.get_active_transfers() != 0)
			{
				long curl_timeout = 0;
				multi.timeout(&curl_timeout);

				int numfds = 0;
				multi.wait(nullptr, 0, curl_timeout, &numfds);

				if (!running_)
					break;

				while (!multi.perform());

				handle_curl_messages(multi);

				handle_cancelled_task(multi);

				if (running_ && handle_new_task(multi))
				{
					while (!multi.perform());
					handle_curl_messages(multi);
				}
				
			}
		}
		
	}
	catch (curl::curl_easy_exception &error)
	{
		curl::curlcpp_traceback errors = error.get_traceback();
		std::for_each(errors.begin(), errors.end(), [](const curl::curlcpp_traceback_object &value) {
			LOG(ERROR) << "curl_easy_exception: " << value.first << " ::::: FUNCTION: " << value.second;
		});

		LOG(FATAL) << "exception occur. app can not work correctly now...";
	}

}

bool NetworkTaskDispatcher::handle_new_task(curl::curl_multi &multi)
{
	int active_trans = multi.get_active_transfers();
	if(active_trans >= MAX_DL_TASK_NUM)
		return false;

	std::list<std::shared_ptr<NetworkTask>> ready_to_schedule_tasks;

	{
		std::lock_guard<std::recursive_mutex> guard(lock_);
		for (auto i = 0; i < MAX_DL_TASK_NUM - active_trans;i++)
		{
			if (need_schedule_tasks_.empty())
				break;

			auto task = need_schedule_tasks_.front();
			need_schedule_tasks_.pop_front();
			ready_to_schedule_tasks.push_back(task);
		}
	}

	if (ready_to_schedule_tasks.empty())
		return false;
	
	for(auto& task : ready_to_schedule_tasks)
	{
		if (!task->on_before_schedule())
			emit_network_task_stop(task);
		else
		{
			multi.add(task->easy());
			schedule_tasks_[task->easy().get_curl()] = task;
			emit_network_task_start(task);
		}
		
	}

	return true;
}

void NetworkTaskDispatcher::handle_curl_messages(curl::curl_multi &multi)
{
	CURLMsg *message = nullptr;
	int message_queued = 0;
	auto curl = multi.get_curl();
	while ((message = curl_multi_info_read(curl, &message_queued)))
	{
		if (message->msg == CURLMSG_DONE)
		{
			auto it = schedule_tasks_.find(message->easy_handle);
			if (it != schedule_tasks_.end())
			{
				auto task = it->second;
				multi.remove(task->easy());
				schedule_tasks_.erase(it);
				task->set_curl_result(message->data.result);
				task->on_after_schedule();
				emit_network_task_stop(task);
			}
			else
			{
				DCHECK(0);
			}
		}
	}
}

void NetworkTaskDispatcher::emit_network_task_start(std::shared_ptr<NetworkTask>& task)
{
	auto obj = this->shared_from_this();
	thread_task_dispatch_->post_task(make_closure([obj, task]()
	{
		task->set_state(NetworkTask::kRunning);
		task->state_changed(task, NetworkTask::kRunning);
		obj->task_state_changed(task);
	}), true);
}

void NetworkTaskDispatcher::emit_network_task_stop(std::shared_ptr<NetworkTask>& task)
{
	auto obj = this->shared_from_this();
	thread_task_dispatch_->post_task(make_closure([obj, task]()
	{
		task->set_state(NetworkTask::kStop);
		task->state_changed(task, NetworkTask::kStop);
		obj->task_state_changed(task);
	}), true);
}

NetworkTaskDispatcher* NetworkTaskDispatcher::global()
{
	static std::shared_ptr<NetworkTaskDispatcher> inst = std::make_shared<NetworkTaskDispatcher>();
	return inst.get();
}

void NetworkTaskDispatcher::handle_cancelled_task(curl::curl_multi &multi)
{
	{
		std::lock_guard<std::recursive_mutex> guard(lock_);
		for (auto it = need_schedule_tasks_.begin(); it != need_schedule_tasks_.end();)
		{
			auto task = *it;
			if (task->is_cancelled())
			{
				it = need_schedule_tasks_.erase(it);
				//task->on_after_schedule(); //这里不应该调用这个函数
				emit_network_task_stop(task);
			}
			else
				++it;
		}
	}

	for (auto it = schedule_tasks_.begin(); it != schedule_tasks_.end();)
	{
		auto task = it->second;
		if (task->is_cancelled())
		{
			multi.remove(task->easy());
			it = schedule_tasks_.erase(it);
			task->on_after_schedule();
			emit_network_task_stop(task);
		}
		else
			++it;
	}
}


