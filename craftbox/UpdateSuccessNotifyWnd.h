#pragma once

#include "BaseShadowWindow.h"

#include <string>

class UpdateSuccessNotifyWnd
	: public BaseShadowWindow
{
public:
	UpdateSuccessNotifyWnd();
	virtual ~UpdateSuccessNotifyWnd();

	void set_updated_version(const std::wstring &ver) { ver_ = ver; }
	void set_updated_note(const std::wstring &note) { note_ = note; }
protected:

	BEGIN_BASE_MSG_MAP
		BASE_MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
		BASE_MESSAGE_HANDLER(WM_NCHITTEST, OnNcHitTest)
	CHAIN_BASE_MSG_MAP(BaseShadowWindow)

	virtual LRESULT OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual void InitWindow() override;
	virtual void Notify(DuiLib::TNotifyUI& msg) override;

private:

	DuiLib::CButtonUI *close_btn_ = nullptr;
	DuiLib::CButtonUI *ok_btn_ = nullptr;
	DuiLib::CLabelUI *tips_ = nullptr;
	DuiLib::CRichEditUI *note_edit_ = nullptr;
	std::wstring ver_;
	std::wstring note_;
};

