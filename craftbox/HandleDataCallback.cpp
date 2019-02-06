#include "HandleDataCallback.h"
#include "base/asynchronous_task_dispatch.h"
#include "glog/logging.h"

HandleDataCallback::HandleDataCallback(HANDLE read_handle)
{
	read_handle_ = read_handle;
}


HandleDataCallback::~HandleDataCallback()
{
	stop();
	join();

	if (read_handle_)
		::CloseHandle(read_handle_);
}

void HandleDataCallback::stop()
{
	stop_ = true;
}

void HandleDataCallback::join()
{
	if (thread_.joinable())
		thread_.join();
}

void HandleDataCallback::start()
{
	if (thread_.joinable())
		return;
	
	stop_ = false;

	std::weak_ptr<HandleDataCallback> weak_obj = this->shared_from_this();
	std::thread t([weak_obj]()
	{
		auto obj = weak_obj.lock();
		if (!obj)
			return;

		while (!obj->stop_)
		{
			DWORD bufsize = 1024;
			DWORD readed_number = 0;
			char buffer[1024];
			if (0 != ReadFile(obj->read_handle_, buffer, bufsize, &readed_number, nullptr))
			{
				std::string recv_buf(buffer, readed_number);
				//::OutputDebugStringA(recv_buf.c_str());
				AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj, recv_buf]()
				{
					auto obj = weak_obj.lock();
					if (!!obj)
					{
						obj->data_available(recv_buf);
					}
				}), true);
			}
			else
			{
				LOG(ERROR) << "readfile from " << obj->read_handle_ << "failed with err " << ::GetLastError();
				::Sleep(200);
			}
		}
	});
	thread_ = std::move(t);
}
