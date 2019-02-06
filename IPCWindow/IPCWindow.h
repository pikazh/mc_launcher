#pragma once

#include <atlbase.h>
#include <atlwin.h>
#include <memory>

#include "base/sigslot.h"
#include "ipccommon.h"

class IPCWindow
	: public  CWindowImpl<IPCWindow>
{
public:
	DECLARE_WND_CLASS(L"craftbox_ipcwin")

	IPCWindow();
	virtual ~IPCWindow();

	BEGIN_MSG_MAP(IPCWindow)
		MESSAGE_HANDLER(WM_COPYDATA, OnCopyData)
	END_MSG_MAP()

	virtual void OnFinalMessage(_In_ HWND /*hWnd*/)
	{
		::PostQuitMessage(0);
	}

	bool init();

	sigslot::signal1<std::shared_ptr<IPCDATA>> msg_recv;

protected:
	LRESULT OnCopyData(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
};

