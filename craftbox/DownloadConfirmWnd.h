#pragma once

#include "Duilib/UIlib.h"

class ShadowEdge;

class DownloadConfirmWnd
	: public DuiLib::CWindowWnd
	, public DuiLib::INotifyUI
{
public:
	DownloadConfirmWnd() = default;
	virtual ~DownloadConfirmWnd() = default;

public:
	bool Init(HWND parent_wnd);
	LPCTSTR GetWindowClassName() const { return _T("DownloadConfirmWin"); }
	void ShowWindow(bool bShow = true, bool bTakeFocus = true);
	std::wstring selected_download_path() { return selected_dl_path_; }
	void show_tips(bool show);
	void set_title(const std::wstring &title);
protected:
	void InitWindow();

	HWND Create(HWND hwndParent, LPCTSTR pstrName, DWORD dwStyle, DWORD dwExStyle, int x = CW_USEDEFAULT, int y = CW_USEDEFAULT, int cx = CW_USEDEFAULT, int cy = CW_USEDEFAULT, HMENU hMenu = NULL);

	virtual LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual LRESULT OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual LRESULT OnMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual LRESULT OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	virtual void Notify(DuiLib::TNotifyUI& msg) override;

	DuiLib::CPaintManagerUI * GetPaintManager() { return &m_pm; }

private:
	DuiLib::CPaintManagerUI m_pm;
	ShadowEdge* m_pShadowWnd = nullptr;

	DuiLib::CLabelUI *tips_ = nullptr;
	DuiLib::CButtonUI *close_btn_ = nullptr;
	DuiLib::COptionUI *dl_in_current_exe_dir_ = nullptr;
	DuiLib::COptionUI *dl_in_specific_dir_ = nullptr;
	DuiLib::CEditUI *dl_path_edit_ = nullptr;
	DuiLib::CButtonUI *dl_path_select_btn_ = nullptr;
	DuiLib::CButtonUI *ok_btn_ = nullptr;
	DuiLib::CButtonUI *cancel_btn_ = nullptr;
	std::wstring selected_dl_path_;
	std::wstring title_;
};

