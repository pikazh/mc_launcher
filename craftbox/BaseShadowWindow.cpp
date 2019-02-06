#include "BaseShadowWindow.h"

BaseShadowWindow::BaseShadowWindow(const std::wstring &xml_file)
	: BaseWindow(xml_file)
{
}


BaseShadowWindow::~BaseShadowWindow()
{
}

HWND BaseShadowWindow::Create(HWND hwndParent, LPCTSTR pstrName, DWORD dwStyle, DWORD dwExStyle, int x /*= CW_USEDEFAULT*/, int y /*= CW_USEDEFAULT*/, int cx /*= CW_USEDEFAULT*/, int cy /*= CW_USEDEFAULT*/, HMENU hMenu /*= NULL*/)
{
	if (m_pShadowWnd == NULL)
		m_pShadowWnd = new ShadowEdge;
	m_pShadowWnd->Create(hwndParent, _T(""), WS_VISIBLE | WS_POPUP | WS_CLIPSIBLINGS, WS_EX_LAYERED | WS_EX_TOOLWINDOW, x, y, cx, cy, NULL);

	return __super::Create(hwndParent, pstrName, dwStyle, dwExStyle, x, y, cx, cy, hMenu);

}

void BaseShadowWindow::ShowWindow(bool bShow, bool bTakeFocus)
{
	if (m_pShadowWnd != NULL)
		m_pShadowWnd->ShowWindow(bShow, false);
	__super::ShowWindow(bShow, bTakeFocus);
}

LRESULT BaseShadowWindow::OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	WORD val = LOWORD(wParam);
	if (val != WA_INACTIVE)
	{
		if (m_pShadowWnd != nullptr)
		{
			RECT rcWnd = { 0 };
			::GetWindowRect(m_hWnd, &rcWnd);
			::SetWindowPos(*m_pShadowWnd, m_hWnd, rcWnd.left, rcWnd.top, rcWnd.right - rcWnd.left,
				rcWnd.bottom - rcWnd.top, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);
		}
	}

	return 0;
}

LRESULT BaseShadowWindow::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
	if (m_pShadowWnd != NULL)
	{
		m_pShadowWnd->Close();
		m_pShadowWnd = nullptr;
	}
		
	bHandled = FALSE;
	return 0;
}

LRESULT BaseShadowWindow::OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
	if ((HWND)wParam == m_hWnd)
		bHandled = TRUE;

	else if (m_pShadowWnd != NULL && (HWND)wParam == m_pShadowWnd->GetHWND())
	{
		CWindowWnd::HandleMessage(uMsg, wParam, lParam);
		::SetFocus(m_hWnd);
		bHandled = TRUE;
	}
	else
	{
		bHandled = FALSE;
	}
	return 0;
}

LRESULT BaseShadowWindow::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
	if (!::IsIconic(*this)) {
		DuiLib::CDuiRect rcWnd;
		::GetWindowRect(*this, &rcWnd);
		rcWnd.Offset(-rcWnd.left, -rcWnd.top);
		HRGN hRgn = ::CreateRectRgn(rcWnd.left + 6, rcWnd.top + 6, rcWnd.right - 6, rcWnd.bottom - 6);
		::SetWindowRgn(*this, hRgn, TRUE);
		::DeleteObject(hRgn);
	}
	if (m_pShadowWnd != NULL)
	{
		RECT rcWnd = { 0 };
		::GetWindowRect(m_hWnd, &rcWnd);
		::SetWindowPos(*m_pShadowWnd, m_hWnd, rcWnd.left, rcWnd.top, rcWnd.right - rcWnd.left,
			rcWnd.bottom - rcWnd.top, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
	}
	bHandled = FALSE;
	return 0;
}

LRESULT BaseShadowWindow::OnMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
	if (m_pShadowWnd != NULL)
	{
		RECT rcWnd = { 0 };
		::GetWindowRect(m_hWnd, &rcWnd);
		::SetWindowPos(*m_pShadowWnd, m_hWnd, rcWnd.left, rcWnd.top, rcWnd.right - rcWnd.left,
			rcWnd.bottom - rcWnd.top, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
	}
	bHandled = FALSE;
	return 0;
}
