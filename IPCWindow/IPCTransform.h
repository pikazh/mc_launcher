#pragma once
#include <windows.h>
#include <stdint.h>
#include <list>
#include <memory>
#include <mutex>
#include <thread>

#include "ipccommon.h"

class IPCTransform
{
public:
	IPCTransform(HWND recv_wnd);
	~IPCTransform();

	void send_ipc_message(uint32_t ipc_msg, const char* data, uint32_t data_len);
	HWND recv_wnd() { return recv_wnd_; }
	void stop() { stop_thread_ = true; }
private:

	std::list<std::shared_ptr<IPCDATA>> ipc_datas_;
	std::recursive_mutex lock_;
	std::thread send_thread_;
	std::thread recv_thread_;
	volatile bool stop_thread_ = false;
	HWND recv_wnd_ = 0;
};

