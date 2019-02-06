#pragma once

#include "MenuWnd.h"

class JavaComboMenu
	: public MenuWnd
{
public:
	JavaComboMenu();
	virtual ~JavaComboMenu();

protected:
	virtual LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) override;
	virtual void Notify(DuiLib::TNotifyUI& msg) override;
};

