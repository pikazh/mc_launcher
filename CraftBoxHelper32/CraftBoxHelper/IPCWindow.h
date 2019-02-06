#pragma once

#include <atlbase.h>
#include <atlwin.h>
#include <memory>

#include "ipccommon.h"

class IPCWindow
	: public  CWindowImpl<IPCWindow>
{
public:
	DECLARE_WND_CLASS(L"craftbox_ipcwin")

	IPCWindow( void 
		(*recv_func)(
		IPCDATA &), void 
		(*timer_func)(
		UINT_PTR));
	virtual ~IPCWindow();

	BEGIN_MSG_MAP(IPCWindow)
		MESSAGE_HANDLER(WM_COPYDATA, OnCopyData)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
	END_MSG_MAP()

	virtual void OnFinalMessage(_In_ HWND /*hWnd*/)
	{
		::PostQuitMessage(0);
	}

	bool init();
	void beginTimer(UINT_PTR id, UINT delay);

protected:
	LRESULT OnCopyData(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
	LRESULT OnTimer(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
private:
	void 
		(*recv_func_)(
		IPCDATA &);

	void 
		(*timer_func_)(
		UINT_PTR);
};

