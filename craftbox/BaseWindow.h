#pragma once

#include "Duilib/UIlib.h"
#include "base/sigslot.h"

#define BEGIN_BASE_MSG_MAP	virtual LRESULT OnMessageMap(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)	\
{	\
	LRESULT ret = 0;	\
	switch(uMsg)	\
	{	\

#define END_BASE_MSG_MAP	\
	default:	\
		bHandled = FALSE;	\
	}	\
	return ret;	\
}

#define CHAIN_BASE_MSG_MAP(cls)	\
	default:	\
		ret = cls::OnMessageMap(uMsg, wParam, lParam, bHandled);	\
	}	\
	return ret;	\
}

#define BASE_MESSAGE_HANDLER(msg, handler)	\
	case msg:	\
	ret = handler(uMsg, wParam, lParam, bHandled);	\
	break;

class BaseWindow
	: public DuiLib::CWindowWnd
	, public DuiLib::INotifyUI
	, public sigslot::has_slots<>
{
public:
	BaseWindow(const std::wstring &xml_file);
	virtual ~BaseWindow() = default;
	
protected:
	virtual void InitWindow();

	BEGIN_BASE_MSG_MAP
		BASE_MESSAGE_HANDLER(WM_CREATE, OnCreate)
		BASE_MESSAGE_HANDLER(WM_CLOSE, OnClose)
	END_BASE_MSG_MAP

	virtual LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	virtual LPCTSTR GetWindowClassName() const override { return _T("BaseWindow"); }

	virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	virtual void Notify(DuiLib::TNotifyUI& msg) override;

	DuiLib::CPaintManagerUI * GetPaintManager() { return &m_pm; }

private:
	DuiLib::CPaintManagerUI m_pm;
	std::wstring xml_file_;
};

