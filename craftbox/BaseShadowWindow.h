#pragma once

#include "BaseWindow.h"
#include "ShadowEdge.h"

class BaseShadowWindow
	: public BaseWindow
{
public:
	BaseShadowWindow(const std::wstring &xml_file);
	virtual ~BaseShadowWindow();

public:
	HWND Create(HWND hwndParent, LPCTSTR pstrName, DWORD dwStyle, DWORD dwExStyle, int x = CW_USEDEFAULT, int y = CW_USEDEFAULT, int cx = CW_USEDEFAULT, int cy = CW_USEDEFAULT, HMENU hMenu = NULL);
	void ShowWindow(bool bShow = true, bool bTakeFocus = true);

protected:

	BEGIN_BASE_MSG_MAP
	BASE_MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
	BASE_MESSAGE_HANDLER(WM_ACTIVATE, OnActivate)
	BASE_MESSAGE_HANDLER(WM_SIZE, OnSize)
	BASE_MESSAGE_HANDLER(WM_MOVE, OnMove)
	CHAIN_BASE_MSG_MAP(BaseWindow)

	virtual LPCTSTR GetWindowClassName() const override { return _T("BaseShadowWindow"); }

	virtual LRESULT OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual LRESULT OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
	virtual LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
	virtual LRESULT OnMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);

private:
	ShadowEdge* m_pShadowWnd = nullptr;
};

