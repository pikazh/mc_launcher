#pragma once

#include "BaseShadowWindow.h"

#include <string>

class ModifyNoteWnd
	: public BaseShadowWindow
{
public:
	ModifyNoteWnd();
	virtual ~ModifyNoteWnd();

public:
	void set_base_dir(const std::wstring &base_dir) { base_dir_ = base_dir; }
	void set_version_str(const std::wstring &version_str) { version_str_ = version_str; }
	void set_old_note(const std::wstring &note) { note_ = note; }
	std::wstring get_note() { return note_; }

protected:

	BEGIN_BASE_MSG_MAP
		BASE_MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
	CHAIN_BASE_MSG_MAP(BaseShadowWindow)

	LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual void InitWindow() override;
	virtual void Notify(DuiLib::TNotifyUI& msg) override;
private:
	std::wstring base_dir_;
	std::wstring version_str_;
	std::wstring note_;

	DuiLib::CLabelUI *title_ = nullptr;
	DuiLib::CLabelUI *path_ = nullptr;
	DuiLib::CEditUI *note_edit_ = nullptr;
	DuiLib::CButtonUI *close_btn_ = nullptr;
	DuiLib::CButtonUI *ok_btn_ = nullptr;
	DuiLib::CButtonUI *cancel_btn_ = nullptr;
};

