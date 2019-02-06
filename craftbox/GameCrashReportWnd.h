#pragma once

#include "BaseShadowWindow.h"

class GameCrashReportWnd
	: public BaseShadowWindow
{
public:
	GameCrashReportWnd();
	virtual ~GameCrashReportWnd();

	void setGameDir(const std::wstring &base_dir, const std::wstring &version_dir);
protected:

	BEGIN_BASE_MSG_MAP
		BASE_MESSAGE_HANDLER(WM_NCHITTEST, OnNcHitTest)
	CHAIN_BASE_MSG_MAP(BaseShadowWindow)

	virtual LRESULT OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual void InitWindow() override;
	virtual void Notify(DuiLib::TNotifyUI& msg) override;

private:

	DuiLib::CButtonUI *close_btn_ = nullptr;
	DuiLib::CButtonUI *ok_btn_ = nullptr;
	DuiLib::CRichEditUI *note_edit_ = nullptr;
	DuiLib::CButtonUI *game_log_btn_ = nullptr;
	std::wstring base_dir_;
	std::wstring version_dir_;
};

