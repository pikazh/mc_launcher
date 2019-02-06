#pragma once

#include "Duilib/UIlib.h"

class ShadowEdge : public DuiLib::CWindowWnd
{
public:
	ShadowEdge()
	{
		m_di.sDrawString = _T("file='menu_bg.png' corner='8,8,8,8'");
	};
	virtual ~ShadowEdge() = default;

	LPCTSTR GetWindowClassName() const { return _T("UIShadow"); };
	UINT GetClassStyle() const { return UI_CLASSSTYLE_FRAME; };
	
	void RePaint();

protected:
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void OnFinalMessage(HWND wnd) override;

private:
	bool m_bNeedUpdate = true;
	DuiLib::TDrawInfo m_di;
	DuiLib::CPaintManagerUI m_pm;
};

