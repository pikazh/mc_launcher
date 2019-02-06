#pragma once

#include "BaseShadowWindow.h"

#include <string>

class QuitConfirmWnd
	: public BaseShadowWindow
{
public:
	QuitConfirmWnd();
	virtual ~QuitConfirmWnd();

public:

	enum Action
	{
		kNotSelect = 0,
		kClose,
		kMinial,
	};

	Action get_select_action() { return select_action_; }
	bool is_always_action() { return is_always_; }

protected:

	BEGIN_BASE_MSG_MAP
		BASE_MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
	CHAIN_BASE_MSG_MAP(BaseShadowWindow)

	LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual void InitWindow() override;
	virtual void Notify(DuiLib::TNotifyUI& msg) override;

	void save_info();
private:
	Action select_action_ = kNotSelect;
	bool is_always_ = false;

	DuiLib::COptionUI *minial_opt_ = nullptr;
	DuiLib::COptionUI *close_opt_ = nullptr;
	DuiLib::COptionUI *always_opt_ = nullptr;

	DuiLib::CButtonUI *close_btn_ = nullptr;
	DuiLib::CButtonUI *ok_btn_ = nullptr;
};

