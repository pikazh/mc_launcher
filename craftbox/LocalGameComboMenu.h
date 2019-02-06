#pragma once

#include "MenuWnd.h"

class LocalGameComboMenu
	: public MenuWnd
{
public:
	LocalGameComboMenu();
	virtual ~LocalGameComboMenu();

protected:
	virtual LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) override;
	virtual void Notify(DuiLib::TNotifyUI& msg) override;
};
