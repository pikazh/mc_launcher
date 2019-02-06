#include "QuitConfirmWnd.h"
#include "glog/logging.h"
#include "AppConfig.h"

QuitConfirmWnd::QuitConfirmWnd()
	: BaseShadowWindow(L"quit_confirm.xml")
{
}

QuitConfirmWnd::~QuitConfirmWnd()
{
}

LRESULT QuitConfirmWnd::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (wParam == VK_ESCAPE)
		Close();
	else
		bHandled = FALSE;
	return 0;
}

void QuitConfirmWnd::InitWindow()
{
	close_btn_ = (DuiLib::CButtonUI*)this->GetPaintManager()->FindControl(L"Close");
	minial_opt_ = (DuiLib::COptionUI*)this->GetPaintManager()->FindControl(L"min_on_close");
	close_opt_ = (DuiLib::COptionUI*)this->GetPaintManager()->FindControl(L"quit_app_on_close");
	always_opt_ = (DuiLib::COptionUI*)this->GetPaintManager()->FindControl(L"alywas");
	ok_btn_ = (DuiLib::CButtonUI*)this->GetPaintManager()->FindControl(L"ok");

	DCHECK(always_opt_ && close_btn_ && minial_opt_ && close_opt_ && ok_btn_ );

	auto on_close = AppConfig::global()->get_value_for_key(L"on_close");
	if (on_close == L"2")
	{
		close_opt_->Selected(true);
	}
	else
	{
		minial_opt_->Selected(true);
	}
}

void QuitConfirmWnd::Notify(DuiLib::TNotifyUI& msg)
{
	if (msg.sType == L"click")
	{
		if (msg.pSender == close_btn_)
		{
			Close();
		}
		else if (msg.pSender == ok_btn_)
		{
			save_info();
			Close();
		}
	}
	else if (msg.sType.Compare(L"return") == 0)
	{
		save_info();
		Close();
	}
	else
		__super::Notify(msg);
}

void QuitConfirmWnd::save_info()
{
	select_action_ = minial_opt_->IsSelected() ? kMinial : kClose;
	is_always_ = always_opt_->IsSelected();
}
