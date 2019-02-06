#include "IPCWindow.h"
#include "ipccommon.h"
#include "glog\logging.h"

#include <windows.h>
#include <memory>
#include <string>


IPCWindow::IPCWindow()
{
}

IPCWindow::~IPCWindow()
{
}

bool IPCWindow::init()
{
	
	HWND wnd = this->Create(HWND_MESSAGE, 0, std::to_wstring(::GetCurrentProcessId()).c_str());
	if (wnd == 0)
		return false;

	typedef BOOL 
		(WINAPI *PChangeWindowMessageFilterEx)(
			__in HWND hwnd,                                         // Window
			__in UINT message,                                      // WM_ message
			__in DWORD action,                                      // Message filter action value
			__inout_opt PCHANGEFILTERSTRUCT pChangeFilterStruct);
	PChangeWindowMessageFilterEx func = (PChangeWindowMessageFilterEx)GetProcAddress(GetModuleHandleW(L"User32.dll"), "ChangeWindowMessageFilterEx");
	if (func)
	{
		if (0 == func(wnd, WM_COPYDATA, MSGFLT_ALLOW, nullptr))
		{
			LOG(ERROR) << "ChangeWindowMessageFilterEx failed ";
		}
	}
	else
	{
		LOG(WARNING) << "can not find ChangeWindowMessageFilterEx ";
	}

	return true;
}

LRESULT IPCWindow::OnCopyData(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
	if (lParam == 0)
		return 0;

	COPYDATASTRUCT *cp = (COPYDATASTRUCT*)lParam;
	if (cp->dwData != IPC_MAGIC_NUM)
		return 0;

	std::shared_ptr<IPCDATA> data(new IPCDATA);
	data->msg = *(uint32_t*)cp->lpData;
	data->data.assign((char*)cp->lpData+sizeof(uint32_t), cp->cbData-sizeof(uint32_t));

	this->msg_recv(data);

	return 0;
}
