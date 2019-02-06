#include "UpdateSuccessNotifyWnd.h"
#include "glog/logging.h"
#include "AppConfig.h"

UpdateSuccessNotifyWnd::UpdateSuccessNotifyWnd()
	: BaseShadowWindow(L"update_notify.xml")
{
}

UpdateSuccessNotifyWnd::~UpdateSuccessNotifyWnd()
{
}

LRESULT UpdateSuccessNotifyWnd::OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	POINT pt; pt.x = GET_X_LPARAM(lParam); pt.y = GET_Y_LPARAM(lParam);
	::ScreenToClient(*this, &pt);

	auto pControl = this->GetPaintManager()->FindControl(pt);
	if (pControl == nullptr)
		return HTCAPTION;
	LPCTSTR class_name = pControl->GetClass();
	if (_tcscmp(class_name, DUI_CTR_TABLAYOUT) == 0
		|| _tcscmp(class_name, DUI_CTR_TILELAYOUT) == 0
		|| _tcscmp(class_name, DUI_CTR_VERTICALLAYOUT) == 0
		|| _tcscmp(class_name, DUI_CTR_HORIZONTALLAYOUT) == 0
		|| _tcscmp(class_name, DUI_CTR_LABEL) == 0
		)
		return HTCAPTION;

	return HTCLIENT;
}

LRESULT UpdateSuccessNotifyWnd::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (wParam == VK_ESCAPE)
		Close();
	else
		bHandled = FALSE;
	return 0;
}

void UpdateSuccessNotifyWnd::InitWindow()
{
	close_btn_ = (DuiLib::CButtonUI*)this->GetPaintManager()->FindControl(L"Close");
	ok_btn_ = (DuiLib::CButtonUI*)this->GetPaintManager()->FindControl(L"ok");
	tips_ = (DuiLib::CLabelUI *)this->GetPaintManager()->FindControl(L"tips");
	note_edit_ = (DuiLib::CRichEditUI*)this->GetPaintManager()->FindControl(L"note_edit");
	DCHECK(note_edit_ && tips_ && close_btn_ && ok_btn_ );

	note_edit_->SetText(note_.c_str());
	std::wstring tip_string = L"创世神启动器" + ver_ + L"版本已经下载成功，请检查程序所在目录";
	tips_->SetText(tip_string.c_str());
}

void UpdateSuccessNotifyWnd::Notify(DuiLib::TNotifyUI& msg)
{
	if (msg.sType == L"click")
	{
		if (msg.pSender == close_btn_)
		{
			Close();
		}
		else if (msg.pSender == ok_btn_)
		{
			Close();
		}
	}
	else
		__super::Notify(msg);
}
