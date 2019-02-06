#include "IPCTransform.h"

IPCTransform::IPCTransform(HWND recv_wnd)
{
	recv_wnd_ = recv_wnd;
	std::thread t([this]()
	{
		while (!this->stop_thread_)
		{
			::Sleep(500);
			std::list<std::shared_ptr<IPCDATA>> data;
			{
				std::lock_guard<std::recursive_mutex> l(lock_);
				data.swap(this->ipc_datas_);
			}

			for (auto &it : data)
			{
				COPYDATASTRUCT cp;
				cp.dwData = IPC_MAGIC_NUM;
				cp.cbData = sizeof(it->msg) + it->data.length();
				std::unique_ptr<char[]> buf(new char[cp.cbData]);
				memcpy(buf.get(), &it->msg, sizeof(it->msg));
				memcpy(buf.get() + sizeof(it->msg), it->data.c_str(), it->data.length());
				cp.lpData = buf.get();
				::SendMessageW(recv_wnd_, WM_COPYDATA, 0, (LPARAM)&cp);
			}
		}
	});

	send_thread_ = std::move(t);
}

IPCTransform::~IPCTransform()
{
	stop_thread_ = true;
	if (send_thread_.joinable())
		send_thread_.join();
}

void IPCTransform::send_ipc_message(uint32_t ipc_msg, const char* data, uint32_t data_len)
{
	std::shared_ptr<IPCDATA> ipc_data(new IPCDATA);
	ipc_data->msg = ipc_msg;
	ipc_data->data.assign(data, data_len);

	{
		std::lock_guard<std::recursive_mutex> l(lock_);
		ipc_datas_.push_back(ipc_data);
	}
}
