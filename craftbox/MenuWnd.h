#pragma once
#include <windows.h>

#include "Duilib/UIlib.h"
#include "base/sigslot.h"

class ShadowEdge;

class MenuWnd 
	: public DuiLib::CWindowWnd
	, public DuiLib::INotifyUI
{
public:
	MenuWnd() = default;
	virtual ~MenuWnd() = default;

	bool Init(DuiLib::CControlUI* pOwner, POINT pt, DuiLib::CDuiString xml_file);
	void AdjustPostion();
	void AdjustNotifyIconPosition();
    LPCTSTR GetWindowClassName() const { return _T("MenuWnd"); }
	void ShowWindow(bool bShow = true, bool bTakeFocus = true);
	void SetTopMost();

	sigslot::signal1<DuiLib::TNotifyUI&> menu_item_clicked;
	
protected:
	HWND Create(HWND hwndParent, LPCTSTR pstrName, DWORD dwStyle, DWORD dwExStyle, int x = CW_USEDEFAULT, int y = CW_USEDEFAULT, int cx = CW_USEDEFAULT, int cy = CW_USEDEFAULT, HMENU hMenu = NULL);

	virtual LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual LRESULT OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual LRESULT OnMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual LRESULT OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	virtual void OnFinalMessage(HWND wnd) override;
	virtual void Notify(DuiLib::TNotifyUI& msg) override;

	DuiLib::CPaintManagerUI * GetPaintManager() { return &m_pm; }
	DuiLib::CControlUI *Owner() { return m_pOwner; }
private:
	DuiLib::CPaintManagerUI m_pm;
	DuiLib::CControlUI* m_pOwner = nullptr;
	ShadowEdge* m_pShadowWnd = nullptr;
	DuiLib::CDuiString xml_file;
};
