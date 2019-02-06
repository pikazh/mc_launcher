#include "IPCWindow.h"
#include "ipccommon.h"

#include <windows.h>
#include <string>


IPCWindow::IPCWindow(void (*recv_func)( IPCDATA &), void (*timer_func)( UINT_PTR))
{
	recv_func_ = recv_func;
	timer_func_ = timer_func;
}

IPCWindow::~IPCWindow()
{
}

bool IPCWindow::init()
{
	wchar_t process_id_buf[32] = {0};
	_itow(::GetCurrentProcessId(), process_id_buf, 10);
	HWND wnd = this->Create(HWND_MESSAGE, 0, process_id_buf);
	if (wnd == 0)
		return false;

	typedef BOOL 
		(WINAPI *PChangeWindowMessageFilterEx)(
			HWND hwnd,                                         // Window
			UINT message,                                      // WM_ message
			DWORD action,                                      // Message filter action value
			PUINT32 pChangeFilterStruct);
	PChangeWindowMessageFilterEx func = (PChangeWindowMessageFilterEx)GetProcAddress(GetModuleHandleW(L"User32.dll"), "ChangeWindowMessageFilterEx");
	if (func)
	{
		if (0 == func(wnd, WM_COPYDATA, 1, NULL))
		{
			::OutputDebugStringA("ChangeWindowMessageFilterEx failed ");
		}
	}
	else
	{
		::OutputDebugStringA("can not find ChangeWindowMessageFilterEx ");
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

	IPCDATA ipcData;
	ipcData.msg = *(UINT32*)cp->lpData;
	ipcData.data.assign((char*)cp->lpData+sizeof(UINT32), cp->cbData-sizeof(UINT32));

	this->recv_func_(ipcData);

	return 0;
}

LRESULT IPCWindow::OnTimer(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
	this->timer_func_(wParam);
	this->KillTimer(wParam);
	return 0;
}

void IPCWindow::beginTimer(UINT_PTR id, UINT delay)
{
	this->SetTimer(id, delay);
}
