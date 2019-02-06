#include "ModifyNoteWnd.h"
#include "glog/logging.h"

ModifyNoteWnd::ModifyNoteWnd()
	: BaseShadowWindow(L"modify_note_wnd.xml")
{
}

ModifyNoteWnd::~ModifyNoteWnd()
{
}

LRESULT ModifyNoteWnd::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (wParam == VK_ESCAPE)
		Close();
	else
		bHandled = FALSE;
	return 0;
}

void ModifyNoteWnd::InitWindow()
{
	close_btn_ = (DuiLib::CButtonUI*)this->GetPaintManager()->FindControl(L"Close");
	title_ = (DuiLib::CLabelUI*)this->GetPaintManager()->FindControl(L"title");
	note_edit_ = (DuiLib::CEditUI*)this->GetPaintManager()->FindControl(L"note_edit");
	path_ = (DuiLib::CLabelUI*)this->GetPaintManager()->FindControl(L"path");
	ok_btn_ = (DuiLib::CButtonUI*)this->GetPaintManager()->FindControl(L"ok");
	cancel_btn_ = (DuiLib::CButtonUI*)this->GetPaintManager()->FindControl(L"cancel");

	DCHECK(close_btn_ && title_ && note_edit_ && path_ && ok_btn_ && cancel_btn_);

	title_->SetMultiLine(true);
	title_->SetText(std::wstring(L"为 " + version_str_ + L" 更改备注: ").c_str());
	note_edit_->SetText(note_.c_str());
	std::wstring text = base_dir_ + L"\\versions\\" + version_str_;
	path_->SetMultiLine(true);
	path_->SetText(text.c_str());
	path_->SetToolTip(std::wstring(base_dir_ + L"\\versions\\" + version_str_).c_str());
}

void ModifyNoteWnd::Notify(DuiLib::TNotifyUI& msg)
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
			note_ = note_edit_->GetText();
			Close();
		}
	}
	else if (msg.sType.Compare(L"return") == 0)
	{
		if (msg.pSender == note_edit_)
		{
			note_ = note_edit_->GetText();
			Close();
		}
	}
	else
		__super::Notify(msg);
}
