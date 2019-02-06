#pragma once

#include "base/sigslot.h"

#include <windows.h>
#include <memory>
#include <thread>

class HandleDataCallback
	: public std::enable_shared_from_this<HandleDataCallback>
{
public:
	HandleDataCallback(HANDLE read_handle);
	virtual ~HandleDataCallback();

	void start();
	void stop();
	void join();

	sigslot::signal1<const std::string &> data_available;
private:
	volatile bool stop_ = false;
	std::thread thread_;
	HANDLE read_handle_ = nullptr;
};

