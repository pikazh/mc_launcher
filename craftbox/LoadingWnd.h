#pragma once

#include "DuiLib/UIlib.h"
#include "BaseWindow.h"

class LoadingWnd
	: public BaseWindow
{
public:
	LoadingWnd()
		: BaseWindow(L"waiting.xml")
	{

	}

protected:
	BEGIN_BASE_MSG_MAP
	BASE_MESSAGE_HANDLER(WM_CLOSE, OnClose)
	CHAIN_BASE_MSG_MAP(BaseWindow)

	virtual LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};
