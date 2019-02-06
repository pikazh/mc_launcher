#include "GameCrashReportWnd.h"
#include "glog/logging.h"

GameCrashReportWnd::GameCrashReportWnd()
	: BaseShadowWindow(L"gamecrash.xml")
{
}

GameCrashReportWnd::~GameCrashReportWnd()
{
}

void GameCrashReportWnd::setGameDir(const std::wstring &base_dir, const std::wstring &version_dir)
{
	base_dir_ = base_dir;
	version_dir_ = version_dir;
}

LRESULT GameCrashReportWnd::OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
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

void GameCrashReportWnd::InitWindow()
{
	close_btn_ = (DuiLib::CButtonUI*)this->GetPaintManager()->FindControl(L"Close");
	ok_btn_ = (DuiLib::CButtonUI*)this->GetPaintManager()->FindControl(L"ok");
	note_edit_ = (DuiLib::CRichEditUI*)this->GetPaintManager()->FindControl(L"note_edit");
	game_log_btn_ = (DuiLib::CButtonUI *)this->GetPaintManager()->FindControl(L"open_log_dir");
	DCHECK(note_edit_ && close_btn_ && ok_btn_ && game_log_btn_);

	std::wstring text = L"非常抱歉, 我的世界 \"" + version_dir_ + L"\" 游戏进程崩溃了。你可以尝试以下方法来解决问题:\r\n1、 更换Java版本: 从Java1.7切换到Java1.8, 或者从Java1.8切换到Java1.7\r\n2、 32位系统下, 请尽量将Java的最大可用内存调到1024M以下\r\n3、 从游戏日志中定位问题";
	
	LONG lineSpace = (LONG)(0.5 * 20);//X为要设置的行间距
	PARAFORMAT2 pf;
	ZeroMemory(&pf, sizeof(pf));
	pf.cbSize = sizeof(PARAFORMAT2);
	pf.dwMask |= PFM_LINESPACING;
	pf.bLineSpacingRule = 5;
	pf.dyLineSpacing = lineSpace;
	note_edit_->SetParaFormat(pf);

	note_edit_->SetText(text.c_str());
}

void GameCrashReportWnd::Notify(DuiLib::TNotifyUI& msg)
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
		else if (msg.pSender == game_log_btn_)
		{
			::ShellExecuteW(this->GetHWND(), L"open", std::wstring(base_dir_ + L"\\versions\\"+version_dir_+L"\\logs").c_str(), nullptr, nullptr, SW_SHOW);
		}
	}
	else
		__super::Notify(msg);
}
