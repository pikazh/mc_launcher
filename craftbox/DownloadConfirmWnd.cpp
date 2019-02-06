#include "DownloadConfirmWnd.h"
#include "ShadowEdge.h"
#include "glog/logging.h"
#include "base/PathService.h"
#include "AppConfig.h"
#include "base/StringHelper.h"

#include <shlobj.h>
#include <shlwapi.h>

bool DownloadConfirmWnd::Init(HWND parent_wnd)
{
	Create(parent_wnd, NULL, WS_VISIBLE | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_TOOLWINDOW);
	if (m_hWnd == 0)
		return false;
	HWND hWndParent = parent_wnd;
	while (::GetParent(hWndParent) != NULL)
		hWndParent = ::GetParent(hWndParent);
	::ShowWindow(m_hWnd, SW_SHOW);
	::SendMessage(hWndParent, WM_NCACTIVATE, TRUE, 0L);
	return true;
}

void DownloadConfirmWnd::InitWindow()
{
	close_btn_ = static_cast<DuiLib::CButtonUI*>(GetPaintManager()->FindControl(L"Close"));
	dl_in_current_exe_dir_ = static_cast<DuiLib::COptionUI*>(GetPaintManager()->FindControl(L"dl_in_exe_dir"));
	dl_in_specific_dir_ = static_cast<DuiLib::COptionUI*>(GetPaintManager()->FindControl(L"dl_in_spedific_dir"));
	dl_path_edit_ = static_cast<DuiLib::CEditUI*>(GetPaintManager()->FindControl(L"download_path"));
	dl_path_select_btn_ = static_cast<DuiLib::CButtonUI*>(GetPaintManager()->FindControl(L"select_dl_path"));
	ok_btn_ = static_cast<DuiLib::CButtonUI*>(GetPaintManager()->FindControl(L"ok"));
	cancel_btn_ = static_cast<DuiLib::CButtonUI*>(GetPaintManager()->FindControl(L"cancel"));
	tips_ = static_cast<DuiLib::CLabelUI*>(GetPaintManager()->FindControl(L"tips"));
	DuiLib::CLabelUI *exe_dir = static_cast<DuiLib::CLabelUI*>(GetPaintManager()->FindControl(L"exe_dir"));
	DuiLib::CLabelUI *title_label = static_cast<DuiLib::CLabelUI*>(GetPaintManager()->FindControl(L"title"));
	DCHECK(title_label
		&& exe_dir
		&& tips_
		&& close_btn_
		&& dl_in_current_exe_dir_
		&& dl_in_specific_dir_
		&& dl_path_edit_
		&& dl_path_select_btn_
		&& ok_btn_
		&& cancel_btn_
		);

	title_label->SetText(title_.c_str());
	std::wstring exe_dir_str = PathService().exe_dir();
	exe_dir->SetText(exe_dir_str.c_str());
	exe_dir->SetToolTip(exe_dir_str.c_str());

	std::wstring save_dir = AppConfig::global()->get_value_for_key(L"download_save_dir");
	dl_path_edit_->SetText(save_dir.c_str());

	if (AppConfig::global()->get_value_for_key(L"download_save_index").compare(L"1") != 0)
	{
		dl_in_current_exe_dir_->Selected(true, false);
		dl_path_select_btn_->SetEnabled(false);
		dl_path_edit_->SetEnabled(false);
	}
	else
	{
		dl_in_specific_dir_->Selected(true, false);
		dl_path_select_btn_->SetEnabled(true);
		dl_path_edit_->SetEnabled(true);
	}
}

HWND DownloadConfirmWnd::Create(HWND hwndParent, LPCTSTR pstrName, DWORD dwStyle, DWORD dwExStyle, int x /*= CW_USEDEFAULT*/, int y /*= CW_USEDEFAULT*/, int cx /*= CW_USEDEFAULT*/, int cy /*= CW_USEDEFAULT*/, HMENU hMenu /*= NULL*/)
{
	if (m_pShadowWnd == NULL) m_pShadowWnd = new ShadowEdge;
	m_pShadowWnd->Create(hwndParent, _T(""), WS_VISIBLE | WS_POPUP | WS_CLIPSIBLINGS, WS_EX_LAYERED | WS_EX_TOOLWINDOW, x, y, cx, cy, NULL);

	dwExStyle |= WS_EX_TOOLWINDOW;
	return CWindowWnd::Create(hwndParent, pstrName, dwStyle, dwExStyle, x, y, cx, cy, hMenu);
}

void DownloadConfirmWnd::ShowWindow(bool bShow, bool bTakeFocus)
{
	if (m_pShadowWnd != NULL) m_pShadowWnd->ShowWindow(bShow, false);
	CWindowWnd::ShowWindow(bShow, bTakeFocus);
}

LRESULT DownloadConfirmWnd::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
	m_pm.Init(m_hWnd);
	DuiLib::CDialogBuilder builder;
	DuiLib::CControlUI* pRoot = builder.Create(L"download_confirm.xml", (UINT)0, NULL, &m_pm);
	ASSERT(pRoot && "Failed to parse XML");
	m_pm.AttachDialog(pRoot);
	m_pm.AddNotifier(this);
	InitWindow();
	return 0;
}

LRESULT DownloadConfirmWnd::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
	if (m_pShadowWnd != NULL)
		m_pShadowWnd->Close();

	bHandled = FALSE;
	return 0;
}

LRESULT DownloadConfirmWnd::OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
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

LRESULT DownloadConfirmWnd::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
	if (wParam == VK_ESCAPE)
		Close();
	return 0;
}

LRESULT DownloadConfirmWnd::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
	if (!::IsIconic(*this)) {
		DuiLib::CDuiRect rcWnd;
		::GetWindowRect(*this, &rcWnd);
		rcWnd.Offset(-rcWnd.left, -rcWnd.top);
		HRGN hRgn = ::CreateRectRgn(rcWnd.left + 6, rcWnd.top + 6, rcWnd.right - 6, rcWnd.bottom - 6);
		::SetWindowRgn(*this, hRgn, TRUE);
		::DeleteObject(hRgn);
	}
	if (m_pShadowWnd != NULL) {
		RECT rcWnd = { 0 };
		::GetWindowRect(m_hWnd, &rcWnd);
		::SetWindowPos(*m_pShadowWnd, m_hWnd, rcWnd.left, rcWnd.top, rcWnd.right - rcWnd.left,
			rcWnd.bottom - rcWnd.top, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
	}
	bHandled = FALSE;
	return 0;
}

LRESULT DownloadConfirmWnd::OnMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
	if (m_pShadowWnd != NULL) {
		RECT rcWnd = { 0 };
		::GetWindowRect(m_hWnd, &rcWnd);
		::SetWindowPos(*m_pShadowWnd, m_hWnd, rcWnd.left, rcWnd.top, rcWnd.right - rcWnd.left,
			rcWnd.bottom - rcWnd.top, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
	}
	bHandled = FALSE;
	return 0;
}

LRESULT DownloadConfirmWnd::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
	bHandled = FALSE;
	return 0;
}

LRESULT DownloadConfirmWnd::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRes = 0;
	BOOL bHandled = TRUE;
	switch (uMsg) {
	case WM_CREATE:        lRes = OnCreate(uMsg, wParam, lParam, bHandled); break;
	case WM_CLOSE:         lRes = OnClose(uMsg, wParam, lParam, bHandled); break;
	case WM_KILLFOCUS:     lRes = OnKillFocus(uMsg, wParam, lParam, bHandled); break;
	case WM_KEYDOWN:       lRes = OnKeyDown(uMsg, wParam, lParam, bHandled); break;
	case WM_MOUSEWHEEL:    lRes = OnMouseWheel(uMsg, wParam, lParam, bHandled); break;
	case WM_SIZE:          lRes = OnSize(uMsg, wParam, lParam, bHandled); break;
	case WM_MOVE:          lRes = OnMove(uMsg, wParam, lParam, bHandled); break;
	default:
		bHandled = FALSE;
	}
	if (bHandled) return lRes;
	if (m_pm.MessageHandler(uMsg, wParam, lParam, lRes)) return lRes;
	return CWindowWnd::HandleMessage(uMsg, wParam, lParam);
}

void DownloadConfirmWnd::Notify(DuiLib::TNotifyUI & msg)
{
	if (msg.sType == L"click")
	{
		if (msg.pSender == cancel_btn_)
		{
			Close();
		}
		else if (msg.pSender == close_btn_)
		{
			Close();
		}
		else if (msg.pSender == ok_btn_)
		{
			bool dl_in_exe_dir = dl_in_current_exe_dir_->IsSelected();
			if (dl_in_exe_dir)
			{
				selected_dl_path_ = PathService().exe_dir();
				AppConfig::global()->set_value_for_key(L"download_save_index", L"0");

				Close();
			}
			else
			{
				std::wstring path = dl_path_edit_->GetText();
				if (wstring_is_abs_path(path))
				{
					wstring_path_remove_duplicated_separator(path);
					wchar_t *ptr = StrStrIW(path.c_str(), L"\\.minecraft");
					if (ptr)
					{
						path = path.substr(0, ptr - path.c_str());
					}
					dl_path_edit_->SetText(path.c_str());

					if (::PathFileExistsW(path.c_str()) && !::PathIsDirectoryW(path.c_str()))
					{
						::MessageBoxW(this->GetHWND(), L"不合法的下载路径", L"错误", MB_OK | MB_ICONINFORMATION);
					}
					else
					{
						selected_dl_path_ = path;

						AppConfig::global()->set_value_for_key(L"download_save_index", L"1");
						AppConfig::global()->set_value_for_key(L"download_save_dir", path.c_str());

						Close();
					}
					
				}
				else
				{
					::MessageBoxW(this->GetHWND(), L"不合法的下载路径", L"错误", MB_OK | MB_ICONINFORMATION);
				}
			}
		}
		else if (msg.pSender == dl_in_current_exe_dir_)
		{
			dl_path_select_btn_->SetEnabled(false);
			dl_path_edit_->SetEnabled(false);
		}
		else if(msg.pSender == dl_in_specific_dir_)
		{
			dl_path_select_btn_->SetEnabled(true);
			dl_path_edit_->SetEnabled(true);
		}
		else if (msg.pSender == dl_path_select_btn_)
		{
			BROWSEINFOW bifolder;
			wchar_t FileName[MAX_PATH + 1] = { 0 };
			ZeroMemory(&bifolder, sizeof(BROWSEINFO));
			bifolder.hwndOwner = this->GetHWND();
			bifolder.pszDisplayName = FileName;
			bifolder.lpszTitle = L"请选择存放路径";
			bifolder.ulFlags = BIF_NEWDIALOGSTYLE | BIF_DONTGOBELOWDOMAIN | BIF_RETURNONLYFSDIRS;

			LPITEMIDLIST idl = SHBrowseForFolder(&bifolder);
			if (idl && SHGetPathFromIDListW(idl, FileName))
			{
				wchar_t *ptr = StrStrIW(FileName, L"\\.minecraft");
				if (ptr)
				{
					*ptr = 0;
				}
				dl_path_edit_->SetText(FileName);
			}
		}
	}
}

void DownloadConfirmWnd::show_tips(bool show)
{
	tips_->SetVisible(show);
}

void DownloadConfirmWnd::set_title(const std::wstring &title)
{
	title_ = title;
}
