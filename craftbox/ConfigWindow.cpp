#include "ConfigWindow.h"
#include "ShadowEdge.h"
#include "glog/logging.h"
#include "minecraft_common/JavaFinder.h"
#include "minecraft_common/JavaDownload.h"
#include "minecraft_common/MinecraftCommon.h"
#include "base/asynchronous_task_dispatch.h"
#include "base/system_info.h"
#include "JavaComboMenu.h"
#include "AppConfig.h"
#include "DownloadConfirmWnd.h"
#include "DataReport/DataReport.h"

#include <iomanip>
#include <codecvt>
#include <atlstr.h>
#include <atlconv.h>
#include "CraftBoxUserJoinServerInfos.h"


#define UPDATE_DL_PROGRESS_TIMER_ID		(1000)

bool ConfigWindow::Init(HWND parent_wnd)
{
	DataReport::global()->report_event(OPEN_CONFIG_WND_EVENT);
	Create(parent_wnd, NULL, WS_VISIBLE | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_TOOLWINDOW);
	if (m_hWnd == 0)
		return false;
	HWND hWndParent = parent_wnd;
	while (::GetParent(hWndParent) != NULL)
		hWndParent = ::GetParent(hWndParent);
	::ShowWindow(m_hWnd, SW_SHOW);
	::SendMessage(hWndParent, WM_NCACTIVATE, TRUE, 0L);
	return true;
}

void ConfigWindow::InitWindow()
{
	app_setting_opt_ = static_cast<DuiLib::COptionUI*>(GetPaintManager()->FindControl(L"app_setting_option"));
	game_setting_opt_ = static_cast<DuiLib::COptionUI*>(GetPaintManager()->FindControl(L"game_setting_option"));

	setting_tab_ = static_cast<DuiLib::CTabLayoutUI*>(GetPaintManager()->FindControl(L"pages"));

	minial_opt_ = static_cast<DuiLib::COptionUI*>(GetPaintManager()->FindControl(L"min_on_close"));
	close_opt_ = static_cast<DuiLib::COptionUI*>(GetPaintManager()->FindControl(L"quit_app_on_close"));
	
	CHECK(minial_opt_ && close_opt_);

	offline_user_name_edit_ = static_cast<DuiLib::CEditUI*>(GetPaintManager()->FindControl(L"offline_player_name"));
	official_account_edit_ = static_cast<DuiLib::CEditUI*>(GetPaintManager()->FindControl(L"email_addr"));
	official_password_edit_ = static_cast<DuiLib::CEditUI*>(GetPaintManager()->FindControl(L"password"));
	official_login_btn_ = static_cast<DuiLib::CButtonUI*>(GetPaintManager()->FindControl(L"official_login"));
	craft_players_cb_ = static_cast<DuiLib::CComboUI*>(GetPaintManager()->FindControl(L"craft_players"));
	craft_servers_cb_ = static_cast<DuiLib::CComboUI*>(GetPaintManager()->FindControl(L"craft_servers"));
	official_login_tab_ = static_cast<DuiLib::CTabLayoutUI*>(GetPaintManager()->FindControl(L"officail_login_tab"));
	official_player_name_lb_ = static_cast<DuiLib::CLabelUI*>(GetPaintManager()->FindControl(L"official_player_name"));
	official_logout_btn_ = static_cast<DuiLib::CButtonUI*>(GetPaintManager()->FindControl(L"official_logout"));
	game_account_type_cb_ = static_cast<DuiLib::CComboUI*>(GetPaintManager()->FindControl(L"game_account_type"));
	game_account_type_pages_ = static_cast<DuiLib::CTabLayoutUI*>(GetPaintManager()->FindControl(L"game_account_type_pages"));
	CHECK(official_logout_btn_ && official_player_name_lb_ && official_login_tab_ && official_login_btn_ && craft_servers_cb_ && craft_players_cb_ && offline_user_name_edit_ && official_account_edit_ && official_password_edit_&& game_account_type_cb_ && game_account_type_pages_);

	select_java_btn_ = static_cast<DuiLib::CButtonUI*>(GetPaintManager()->FindControl(L"select_java_btn"));
	java_operator_btn_ = static_cast<DuiLib::CButtonUI*>(GetPaintManager()->FindControl(L"java_operator"));
	java_version_label_ = static_cast<DuiLib::CButtonUI*>(GetPaintManager()->FindControl(L"java_version"));
	java_searching_tips_ = static_cast<DuiLib::CButtonUI*>(GetPaintManager()->FindControl(L"java_searching_tips"));
	java_max_avail_mem_size_edit_ = static_cast<DuiLib::CEditUI*>(GetPaintManager()->FindControl(L"max_memory"));
	auto sys_mem_size_label_ = static_cast<DuiLib::CLabelUI*>(GetPaintManager()->FindControl(L"sys_memory"));
	auto java_mem_setting_layout = static_cast<DuiLib::CHorizontalLayoutUI*>(GetPaintManager()->FindControl(L"java_mem_setting_layout"));
	java_download_tips_layout_ = static_cast<DuiLib::CHorizontalLayoutUI*>(GetPaintManager()->FindControl(L"java_download_tips_layout"));
	java_dl_layout_ = static_cast<DuiLib::CHorizontalLayoutUI*>(GetPaintManager()->FindControl(L"java_download_layout"));
	java_dl_progress_ = static_cast<DuiLib::CProgressUI*>(GetPaintManager()->FindControl(L"java_download_progress"));
	cancel_java_dl_btn_ = static_cast<DuiLib::CButtonUI*>(GetPaintManager()->FindControl(L"cancel_java_download"));
	close_btn_ = static_cast<DuiLib::CButtonUI*>(GetPaintManager()->FindControl(L"Close"));
	ok_btn_ = static_cast<DuiLib::CButtonUI*>(GetPaintManager()->FindControl(L"ok_btn"));

	DCHECK(app_setting_opt_
		&& game_setting_opt_
		&& setting_tab_
		&& select_java_btn_
		&& java_operator_btn_
		&& java_version_label_
		&& java_searching_tips_
		&& java_max_avail_mem_size_edit_
		&& sys_mem_size_label_
		&& java_mem_setting_layout
		&& java_dl_layout_
		&& java_download_tips_layout_
		&& java_dl_progress_
		&& cancel_java_dl_btn_
		&& close_btn_
		&& ok_btn_
		);


	offline_user_name_edit_->SetTag(offline_user_name_edit_->GetTextColor());
	auto offline_user_name = AppConfig::global()->get_value_for_key(L"offline_player_name");
	if (!offline_user_name.empty())
		offline_user_name_edit_->SetText(offline_user_name.c_str());
	else
	{
		offline_user_name_edit_->SetAttribute(L"textcolor", L"#FF666666");
		offline_user_name_edit_->SetText(L"请输入玩家名");
	}

	official_account_edit_->SetTag(official_account_edit_->GetTextColor());
	official_account_edit_->SetAttribute(L"textcolor", L"#FF666666");
	official_account_edit_->SetText(L"请输入正版账号");

	official_password_edit_->SetTag(official_password_edit_->GetTextColor());
	official_password_edit_->SetAttribute(L"textcolor", L"#FF666666");
	official_password_edit_->SetText(L"请输入正版账号密码");

	auto login_type = (mcfe::UserAccountType)_wtoi(AppConfig::global()->get_value_for_key(L"account_type").c_str());
	game_account_type_cb_->SelectItem(login_type);


	auto on_close_always = AppConfig::global()->get_value_for_key(L"always_on_close");
	if (on_close_always == L"1")
	{
		auto on_close = AppConfig::global()->get_value_for_key(L"on_close");
		if (on_close == L"2")
		{
			close_opt_->Selected(true);
		}
		else if (on_close == L"1")
		{
			minial_opt_->Selected(true);
		}
	}


	auto java_download = java_env_->download_java();
	java_download->download_java_failed.connect(this, &ConfigWindow::on_java_download_failed);
	java_download->download_java_success.connect(this, &ConfigWindow::on_java_download_success);
	if (java_download->download_step() != JavaDownload::DL_NOT_START)
	{
		java_dl_layout_->SetVisible(true);
		java_download_tips_layout_->SetVisible(true);
		update_download_progress();
		update_dl_progress_timer_ = ::SetTimer(m_hWnd, UPDATE_DL_PROGRESS_TIMER_ID, 500, nullptr);
	}
	else
	{
		java_dl_layout_->SetVisible(false);
		java_download_tips_layout_->SetVisible(false);
	}

	auto java_search = java_env_->search_java();
	java_search->search_cancelled.connect(this, &ConfigWindow::on_java_search_finished);
	java_search->search_finished.connect(this, &ConfigWindow::on_java_search_finished);
	if (java_search->is_searching())
	{
		java_operator_btn_->SetText(L"停止查找");
		java_searching_tips_->SetVisible(true);
	}
	else
	{
		java_operator_btn_->SetText(L"添加或下载");
		java_searching_tips_->SetVisible(false);
	}

	handle_java_list();

	sys_mem_size_label_->SetText(std::wstring(L"系统物理内存: " + std::to_wstring(SystemInfo().total_physical_memory_size() >> 20) + L"MB").c_str());
	java_max_avail_mem_size_edit_->SetText(std::to_wstring(java_env_->java_Xmx_MB()).c_str());

	CraftBoxUserJoinServerInfos *info = CraftBoxUserJoinServerInfos::instance();
	if (info->is_fetch_finished())
		on_fetch_user_join_server_info_finished();
	else
	{
		info->fetch_finished.connect(this, &ConfigWindow::on_fetch_user_join_server_info_finished);
	}

	on_yggdrasil_auth_state_changed(YggdrasilAuthentication::global()->authenticate_state());
}

HWND ConfigWindow::Create(HWND hwndParent, LPCTSTR pstrName, DWORD dwStyle, DWORD dwExStyle, int x /*= CW_USEDEFAULT*/, int y /*= CW_USEDEFAULT*/, int cx /*= CW_USEDEFAULT*/, int cy /*= CW_USEDEFAULT*/, HMENU hMenu /*= NULL*/)
{
	if (m_pShadowWnd == NULL)
		m_pShadowWnd = new ShadowEdge;
	m_pShadowWnd->Create(hwndParent, _T(""), WS_VISIBLE | WS_POPUP | WS_CLIPSIBLINGS, WS_EX_LAYERED | WS_EX_TOOLWINDOW, x, y, cx, cy, NULL);

	return CWindowWnd::Create(hwndParent, pstrName, dwStyle, dwExStyle, x, y, cx, cy, hMenu);
}

void ConfigWindow::ShowWindow(bool bShow, bool bTakeFocus)
{
	if (m_pShadowWnd != NULL) m_pShadowWnd->ShowWindow(bShow, false);
	CWindowWnd::ShowWindow(bShow, bTakeFocus);
}

LRESULT ConfigWindow::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
	m_pm.Init(m_hWnd);
	DuiLib::CDialogBuilder builder;
	DuiLib::CControlUI* pRoot = builder.Create(L"config.xml", (UINT)0, NULL, &m_pm);
	ASSERT(pRoot && "Failed to parse XML");
	m_pm.AttachDialog(pRoot);
	m_pm.AddNotifier(this);

	InitWindow();
	return 0;
}

LRESULT ConfigWindow::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
	if (m_pShadowWnd != NULL)
	{
		m_pShadowWnd->Close();
		m_pShadowWnd = nullptr;
	}

	if (update_dl_progress_timer_ != 0)
	{
		::KillTimer(m_hWnd, update_dl_progress_timer_);
		update_dl_progress_timer_ = 0;
	}

	bHandled = FALSE;
	return 0;
}

LRESULT ConfigWindow::OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
	if ((HWND)wParam == m_hWnd)
		bHandled = TRUE;

	else if (m_pShadowWnd != NULL && (HWND)wParam == m_pShadowWnd->GetHWND())
	{
		CWindowWnd::HandleMessage(uMsg, wParam, lParam);
		::SetFocus(m_hWnd);
		bHandled = TRUE;
	}
	else
	{
		bHandled = FALSE;
	}
	return 0;
}

LRESULT ConfigWindow::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
	if (wParam == VK_ESCAPE)
		Close();
	return 0;
}

LRESULT ConfigWindow::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
	if (!::IsIconic(*this)) {
		DuiLib::CDuiRect rcWnd;
		::GetWindowRect(*this, &rcWnd);
		rcWnd.Offset(-rcWnd.left, -rcWnd.top);
		HRGN hRgn = ::CreateRectRgn(rcWnd.left + 6, rcWnd.top + 6, rcWnd.right - 6, rcWnd.bottom - 6);
		::SetWindowRgn(*this, hRgn, TRUE);
		::DeleteObject(hRgn);
	}
	if (m_pShadowWnd != NULL) {
		RECT rcWnd = { 0 };
		::GetWindowRect(m_hWnd, &rcWnd);
		::SetWindowPos(*m_pShadowWnd, m_hWnd, rcWnd.left, rcWnd.top, rcWnd.right - rcWnd.left,
			rcWnd.bottom - rcWnd.top, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
	}
	bHandled = FALSE;
	return 0;
}

LRESULT ConfigWindow::OnMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
	if (m_pShadowWnd != NULL) {
		RECT rcWnd = { 0 };
		::GetWindowRect(m_hWnd, &rcWnd);
		::SetWindowPos(*m_pShadowWnd, m_hWnd, rcWnd.left, rcWnd.top, rcWnd.right - rcWnd.left,
			rcWnd.bottom - rcWnd.top, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
	}
	bHandled = FALSE;
	return 0;
}

LRESULT ConfigWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRes = 0;
	BOOL bHandled = TRUE;
	switch (uMsg) {
	case WM_CREATE:        lRes = OnCreate(uMsg, wParam, lParam, bHandled); break;
	case WM_CLOSE:         lRes = OnClose(uMsg, wParam, lParam, bHandled); break;
	case WM_KILLFOCUS:     lRes = OnKillFocus(uMsg, wParam, lParam, bHandled); break;
	case WM_KEYDOWN:       lRes = OnKeyDown(uMsg, wParam, lParam, bHandled); break;
	case WM_SIZE:          lRes = OnSize(uMsg, wParam, lParam, bHandled); break;
	case WM_MOVE:          lRes = OnMove(uMsg, wParam, lParam, bHandled); break;
	case WM_TIMER:		   lRes = OnTimer(uMsg, wParam, lParam, bHandled); break;
	case WM_NCHITTEST:	   lRes = OnNcHitTest(uMsg, wParam, lParam, bHandled); break;
	default:
		bHandled = FALSE;
	}
	if (bHandled) return lRes;
	if (m_pm.MessageHandler(uMsg, wParam, lParam, lRes)) return lRes;
	return CWindowWnd::HandleMessage(uMsg, wParam, lParam);
}

void ConfigWindow::do_official_login()
{
	if (!official_password_edit_->IsPasswordMode())
	{
		::MessageBoxW(this->GetHWND(), L"请先输入正版账号和密码", L"提示", MB_ICONINFORMATION | MB_OK);
		return;
	}
	auto account = official_account_edit_->GetText();
	auto password = official_password_edit_->GetText();
	if (account.IsEmpty() || password.IsEmpty())
	{
		::MessageBoxW(this->GetHWND(), L"请先输入正版账号和密码", L"提示", MB_ICONINFORMATION | MB_OK);
		return;
	}

	USES_CONVERSION;
	std::string official_account = ATL::CW2A(account.GetData());
	std::string official_password = ATL::CW2A(password.GetData());
	YggdrasilAuthentication::global()->authenticate(official_account, official_password);
}

void ConfigWindow::Notify(DuiLib::TNotifyUI& msg)
{
	if (msg.sType == L"click")
	{
		if (msg.pSender == close_btn_)
		{
			DataReport::global()->report_event(CLICK_CONFIG_WND_CONTROL, 0);
			Close();
		}
		else if (msg.pSender == ok_btn_)
		{
			DataReport::global()->report_event(CLICK_CONFIG_WND_CONTROL, 1);
			save_config();
			Close();
		}
		else if (msg.pSender == official_login_btn_)
		{
			do_official_login();
		}
		else if (msg.pSender == official_logout_btn_)
		{
			auto yggdrasil_auth = YggdrasilAuthentication::global();
			yggdrasil_auth->invalidate(yggdrasil_auth->access_token(), yggdrasil_auth->client_token());
		}
		else if (msg.pSender == select_java_btn_)
		{
			DataReport::global()->report_event(CLICK_CONFIG_WND_CONTROL, 2);
			JavaComboMenu *menu = new JavaComboMenu;
			auto rect = select_java_btn_->GetPos();
			POINT pt = { rect.left - 4, rect.bottom - 4 };
			::ClientToScreen(*this, &pt);
			menu->Init(select_java_btn_, pt, L"java_combo_list.xml");
			menu->AdjustPostion();
		}
		else if (msg.pSender == java_operator_btn_)
		{
			DataReport::global()->report_event(CLICK_CONFIG_WND_CONTROL, 3);
			auto java_search = java_env_->search_java();
			if (java_search->is_searching())
			{
				java_search->stop_search();
			}
			else
			{
				auto *menu = new MenuWnd;
				auto rect = java_operator_btn_->GetPos();
				POINT pt = { rect.left - 4, rect.bottom - 4 };
				::ClientToScreen(*this, &pt);
				menu->Init(msg.pSender, pt, L"AddJavaBtnMenu.xml");
				menu->AdjustPostion();
			}
			
		}
		else if (msg.pSender == cancel_java_dl_btn_)
		{
			DataReport::global()->report_event(CLICK_CONFIG_WND_CONTROL, 4);
			java_env_->download_java()->stop_download();
		}
	}
	else if (msg.pSender == game_account_type_cb_)
	{
		if (msg.sType == DUI_MSGTYPE_ITEMSELECT)
		{
			game_account_type_pages_->SelectItem(msg.wParam);

			AppConfig::global()->set_value_for_key(L"account_type", std::to_wstring(msg.wParam).c_str());
		}
	}
	else if (msg.pSender == offline_user_name_edit_)
	{
		if (msg.sType == L"killfocus")
		{
			auto text = offline_user_name_edit_->GetText();
			if (text.IsEmpty())
			{
				offline_user_name_edit_->SetAttribute(L"textcolor", L"#FF666666");
				offline_user_name_edit_->SetText(L"请输入玩家名");
			}

			AppConfig::global()->set_value_for_key(L"offline_player_name", text.GetData());
		}
		else if (msg.sType == L"setfocus")
		{
			auto text = offline_user_name_edit_->GetText();
			if (text == L"请输入玩家名")
			{
				offline_user_name_edit_->SetText(L"");
				offline_user_name_edit_->SetTextColor(offline_user_name_edit_->GetTag());
			}
		}
	}
	else if (msg.pSender == official_account_edit_)
	{
		if (msg.sType == L"killfocus")
		{
			auto text = official_account_edit_->GetText();
			if (text.IsEmpty())
			{
				official_account_edit_->SetAttribute(L"textcolor", L"#FF666666");
				official_account_edit_->SetText(L"请输入正版账号");
			}
		}
		else if (msg.sType == L"setfocus")
		{
			auto text = official_account_edit_->GetText();
			if (text == L"请输入正版账号")
			{
				official_account_edit_->SetText(L"");
				official_account_edit_->SetTextColor(official_account_edit_->GetTag());
			}
		}
		else if (msg.sType == L"return")
		{
			do_official_login();
		}
	}
	else if (msg.pSender == official_password_edit_)
	{
		if (msg.sType == L"killfocus")
		{
			auto text = official_password_edit_->GetText();
			if (text.IsEmpty())
			{
				official_password_edit_->SetAttribute(L"textcolor", L"#FF666666");
				official_password_edit_->SetPasswordMode(false);
				official_password_edit_->SetText(L"请输入正版账号密码");
			}
		}
		else if (msg.sType == L"setfocus")
		{
			auto text = official_password_edit_->GetText();
			if (text == L"请输入正版账号密码")
			{
				official_password_edit_->SetText(L"");
				official_password_edit_->SetTextColor(official_account_edit_->GetTag());
				official_password_edit_->SetPasswordMode(true);
			}
		}
		else if (msg.sType == L"return")
		{
			do_official_login();
		}
	}
	else if (msg.pSender == craft_servers_cb_)
	{
		if (msg.sType == DUI_MSGTYPE_ITEMSELECT)
		{
			craft_players_cb_->RemoveAll();
			
			auto item = craft_servers_cb_->GetItemAt(craft_servers_cb_->GetCurSel());
			if (item)
			{
				auto sid = item->GetCustomAttribute(L"sid");
				if (sid)
				{
					CraftBoxUserJoinServerInfos* inst = CraftBoxUserJoinServerInfos::instance();
					auto &join_server_infos = inst->join_server_infos();
					std::wstring pid = inst->select_pid();
					
					auto it = join_server_infos.find(sid);
					if (it != join_server_infos.end())
					{
						inst->set_selected_sid(sid);
						int i = 0;
						DuiLib::CListLabelElementUI* pSelectListElement = nullptr;
						auto &players = it->second->players;
						for (auto it2 = players.begin(); it2 != players.end(); ++it2)
						{
							DuiLib::CListLabelElementUI* pListElement = nullptr;
							if (!default_list_label_builder_.GetMarkup()->IsValid())
							{
								pListElement = static_cast<DuiLib::CListLabelElementUI*>(default_list_label_builder_.Create(_T("default_listlabelelement.xml"), (UINT)0, nullptr, GetPaintManager()));
							}
							else
							{
								pListElement = static_cast<DuiLib::CListLabelElementUI*>(default_list_label_builder_.Create((UINT)0, GetPaintManager()));
							}
							DCHECK(pListElement);

							pListElement->AddCustomAttribute(L"pid", (*it2)->pid.c_str());
							std::wstring text = (*it2)->player_name;
							std::string &ban = (*it2)->ban;
							if (ban == "1")
								text += L"(已封禁)";
							else if (ban == "2")
								text += L"(待验证)";

							pListElement->SetText(text.c_str());
							craft_players_cb_->AddAt(pListElement, i++);

							if (!pid.empty() && (*it2)->pid == pid)
							{
								craft_players_cb_->SelectItem(i - 1);
								pSelectListElement = pListElement;
							}
						}

						if (pSelectListElement == nullptr && 0 < i)
							craft_players_cb_->SelectItem(0);
					}
					else
					{
						DCHECK(0);
					}
				}
				else
				{
					DCHECK(0);
				}
			}

		}
	}
	else if (msg.pSender == craft_players_cb_)
	{
		if (msg.sType == DUI_MSGTYPE_ITEMSELECT)
		{
			bool selected = false;
			auto item = craft_players_cb_->GetItemAt(craft_players_cb_->GetCurSel());
			if (item)
			{
				auto pid = item->GetCustomAttribute(L"pid");
				if (pid != nullptr)
				{
					selected = true;
					CraftBoxUserJoinServerInfos::instance()->set_selected_pid(pid);
				}
					
			}
			if (!selected)
			{
				CraftBoxUserJoinServerInfos::instance()->set_selected_pid(L"");
			}
		}
	}
	else if (msg.pSender == select_java_btn_)
	{
		if (msg.sType == L"itemclick")
		{
			DataReport::global()->report_event(CLICK_CONFIG_WND_CONTROL, 5);
			auto element = (DuiLib::CListContainerElementUI*)msg.wParam;
			if (element)
			{
				std::wstring path = element->GetCustomAttribute(L"path");
				std::wstring version = element->GetCustomAttribute(L"version");
				select_java_btn_->SetText(path.c_str());
				select_java_btn_->SetToolTip(std::wstring(L"路径: " + path).c_str());
				java_version_label_->SetText(std::wstring(L"版本: " + version).c_str());
				java_version_label_->SetVisible(true);
			}
		}
		
	}
	else if (msg.sType == L"scan_local_java")
	{
		DataReport::global()->report_event(CLICK_CONFIG_WND_CONTROL, 5);
		java_env_->search_java()->begin_search();
		java_operator_btn_->SetText(L"停止查找");
		java_searching_tips_->SetVisible(true);
	}
	else if (msg.sType == L"manual_add_java_path")
	{
		DataReport::global()->report_event(CLICK_CONFIG_WND_CONTROL, 5);
		wchar_t szBuffer[1024] = { 0 };
		OPENFILENAMEW ofn = { 0 };
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = m_hWnd;
		ofn.lpstrTitle = L"请选择Java程序所在路径";
		ofn.lpstrFilter = L"Java程序(Javaw.exe)\0Javaw.exe\0\0";
		ofn.lpstrFile = szBuffer;
		ofn.nMaxFile = 1024;
		ofn.nFilterIndex = 0;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
		if (GetOpenFileNameW(&ofn))
		{
			if (java_env_->try_add_java(szBuffer))
			{
				java_env_->set_select_java_path(szBuffer);
				handle_java_list();
			}
		}
	}
	else if (msg.sType == L"download_java7")
	{
		auto java_dl = java_env_->download_java();
		if (java_dl->download_step() == JavaDownload::DL_NOT_START)
		{
			DataReport::global()->report_event(CLICK_CONFIG_WND_CONTROL, 5);

			DownloadConfirmWnd dl_confirm_wnd;
			dl_confirm_wnd.set_title(L"下载Java1.7");
			dl_confirm_wnd.Init(this->GetHWND());
			dl_confirm_wnd.show_tips(false);
			dl_confirm_wnd.CenterWindow();
			dl_confirm_wnd.ShowModal();
			auto dl_path = dl_confirm_wnd.selected_download_path();
			if (!dl_path.empty())
			{
				java_dl_layout_->SetVisible(true);
				java_download_tips_layout_->SetVisible(true);
				java_dl_progress_->SetValue(0);
				java_dl_progress_->SetText(L"");
				update_dl_progress_timer_ = ::SetTimer(m_hWnd, UPDATE_DL_PROGRESS_TIMER_ID, 500, nullptr);
				java_dl->set_download_dir(dl_path);
				java_dl->start_download(7);
			}
			
		}
	}
	else if (msg.sType == L"download_java8")
	{
		auto java_dl = java_env_->download_java();
		if (java_dl->download_step() == JavaDownload::DL_NOT_START)
		{
			DataReport::global()->report_event(CLICK_CONFIG_WND_CONTROL, 5);

			DownloadConfirmWnd dl_confirm_wnd;
			dl_confirm_wnd.set_title(L"下载Java1.8");
			dl_confirm_wnd.Init(this->GetHWND());
			dl_confirm_wnd.show_tips(false);
			dl_confirm_wnd.CenterWindow();
			dl_confirm_wnd.ShowModal();
			auto dl_path = dl_confirm_wnd.selected_download_path();
			if (!dl_path.empty())
			{
				java_dl_layout_->SetVisible(true);
				java_download_tips_layout_->SetVisible(true);
				java_dl_progress_->SetValue(0);
				java_dl_progress_->SetText(L"");
				update_dl_progress_timer_ = ::SetTimer(m_hWnd, UPDATE_DL_PROGRESS_TIMER_ID, 500, nullptr);
				java_dl->set_download_dir(dl_path);
				java_dl->start_download(8);
			}
		}
	}
	else if (msg.pSender == app_setting_opt_)
	{
		if (msg.sType.Compare(DUI_MSGTYPE_SELECTCHANGED) == 0)
		{
			setting_tab_->SelectItem(1);
		}
	}
	else if (msg.pSender == game_setting_opt_)
	{
		if (msg.sType.Compare(DUI_MSGTYPE_SELECTCHANGED) == 0)
		{
			setting_tab_->SelectItem(0);
		}
	}
}

ConfigWindow::ConfigWindow()
{
	java_env_ = JavaEnvironment::GetInstance();
	YggdrasilAuthentication::global()->authenticate_state_changed.connect(this, &ConfigWindow::on_yggdrasil_auth_state_changed);
}

void ConfigWindow::on_yggdrasil_auth_state_changed(YggdrasilAuthentication::AuthState state)
{
	switch (state)
	{
	case YggdrasilAuthentication::kAuthenticating:
	case YggdrasilAuthentication::kRefreshing:
	case YggdrasilAuthentication::kValidating:
		official_login_tab_->SelectItem(1);
		break;
	case YggdrasilAuthentication::kValidate_failed:
	{
		AsyncTaskDispatch::main_thread()->post_task(make_closure([]()
		{
			auto yggdrasil_auth = YggdrasilAuthentication::global();
			yggdrasil_auth->refresh(yggdrasil_auth->access_token(), yggdrasil_auth->client_token());
		}), true);
		break;
	}

	case YggdrasilAuthentication::kRefresh_failed:
		official_login_tab_->SelectItem(0);
		break;
	case YggdrasilAuthentication::kAuthenticate_failed:
		official_login_tab_->SelectItem(0);
		::MessageBoxW(GetHWND(), YggdrasilAuthentication::global()->err_message().c_str(), L"正版登录认证失败", MB_OK | MB_ICONERROR);
		break;

	case YggdrasilAuthentication::kInvalidated:
	case YggdrasilAuthentication::kInvalidating:
		official_login_tab_->SelectItem(0);
		break;

	case YggdrasilAuthentication::kValidated:
	{
		official_login_tab_->SelectItem(2);
		std::wstring text = L"角色名字: ";
		auto &player_name = YggdrasilAuthentication::global()->player_name();
		text += std::wstring(player_name.begin(), player_name.end());
		official_player_name_lb_->SetText(text.c_str());
		break;
	}

	default:
		break;
	}
}

void ConfigWindow::on_java_search_finished()
{
	java_operator_btn_->SetText(L"添加或下载");
	java_searching_tips_->SetVisible(false);

	AsyncTaskDispatch::current()->post_task(make_closure(&ConfigWindow::handle_java_list, this->shared_from_this()), true);
}

void ConfigWindow::on_java_download_success()
{
	java_dl_layout_->SetVisible(false);
	java_download_tips_layout_->SetVisible(false);
	if (update_dl_progress_timer_ != 0)
	{
		::KillTimer(m_hWnd, update_dl_progress_timer_);
		update_dl_progress_timer_ = 0;
	}

	AsyncTaskDispatch::current()->post_task(make_closure(&ConfigWindow::handle_java_list, this->shared_from_this()), true);
}

void ConfigWindow::on_java_download_failed()
{
	java_dl_layout_->SetVisible(false);
	java_download_tips_layout_->SetVisible(false);
	if (update_dl_progress_timer_ != 0)
	{
		::KillTimer(m_hWnd, update_dl_progress_timer_);
		update_dl_progress_timer_ = 0;
	}

	DataReport::global()->report_event(CLICK_CONFIG_WND_CONTROL, 5);
}

void ConfigWindow::handle_java_list()
{
	auto select_java_path = java_env_->selected_java_path();
	if (select_java_path.empty())
	{
		auto javas = java_env_->java_paths_and_vers();
		auto it = javas.begin();
		if (it != javas.end())
		{
			select_java_path = it->first;
		}
		
	}

	if (select_java_path.empty())
	{
		java_version_label_->SetVisible(false);
		select_java_btn_->SetText(L"");
		select_java_btn_->SetToolTip(L"");
	}
	else
	{
		auto javas = java_env_->java_paths_and_vers();
		auto it = javas.find(select_java_path);
		if (it == javas.end())
		{
			DCHECK(0);
		}
		else
		{
			std::wstring ver = std::wstring(it->second.begin(), it->second.end());
			ver = L"版本: " + ver;
			java_version_label_->SetVisible(true);
			java_version_label_->SetText(ver.c_str());
			select_java_btn_->SetText(it->first.c_str());
			select_java_btn_->SetToolTip(std::wstring(L"路径: " + it->first).c_str());
		}
	}
}

LRESULT ConfigWindow::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (wParam == UPDATE_DL_PROGRESS_TIMER_ID)
	{
		update_download_progress();
	}
	else
		bHandled = false;
	
	return 0;
}

void ConfigWindow::update_download_progress()
{
	auto java_download = java_env_->download_java();

	auto state = java_download->state();
	if (state == NetworkTask::kWaiting)
	{
		java_dl_progress_->SetValue(0);
		java_dl_progress_->SetText(L"下载队列排队中...");
	}
	else
	{
		auto step = java_download->download_step();
		if (step == JavaDownload::DL_UNCOMPRESS)
		{
			java_dl_progress_->SetValue(100);
			java_dl_progress_->SetText(L"Java解压中...");
		}
		else if (step != JavaDownload::DL_NOT_START)
		{
			auto dltotal = java_download->dl_total();
			auto dlcurrent = java_download->dl_current();
			if (dltotal > 0)
				java_dl_progress_->SetValue(100 * dlcurrent / dltotal);
			else
				java_dl_progress_->SetValue(0);

			std::wstringstream ss;
			ss << std::fixed << setprecision(2) << ((double)dlcurrent) / 1024 / 1024 << L"MB/" << std::fixed << setprecision(2) << ((double)dltotal) / 1024 / 1024 << L"MB";
			java_dl_progress_->SetText(ss.str().c_str());
		}
	}
}

void ConfigWindow::save_config()
{
	if (minial_opt_->IsSelected())
	{
		AppConfig::global()->set_value_for_key(L"always_on_close", L"1");
		AppConfig::global()->set_value_for_key(L"on_close", L"1");
	}
	else if (close_opt_->IsSelected())
	{
		AppConfig::global()->set_value_for_key(L"always_on_close", L"1");
		AppConfig::global()->set_value_for_key(L"on_close", L"2");
	}

	java_env_->set_select_java_path(std::wstring(select_java_btn_->GetText()));
	java_env_->set_java_Xmx_MB(_wtoi(java_max_avail_mem_size_edit_->GetText()));
}

LRESULT ConfigWindow::OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	POINT pt; pt.x = GET_X_LPARAM(lParam); pt.y = GET_Y_LPARAM(lParam);
	::ScreenToClient(*this, &pt);

	if (pt.y > 50)
		return HTCLIENT;

	auto pControl = m_pm.FindControl(pt);
	if (pControl == nullptr)
		return HTCAPTION;
	LPCTSTR class_name = pControl->GetClass();
	if (_tcscmp(class_name, DUI_CTR_TABLAYOUT) == 0
		|| _tcscmp(class_name, DUI_CTR_TILELAYOUT) == 0
		|| _tcscmp(class_name, DUI_CTR_VERTICALLAYOUT) == 0
		|| _tcscmp(class_name, DUI_CTR_HORIZONTALLAYOUT) == 0
		)
		return HTCAPTION;

	return HTCLIENT;
}

void ConfigWindow::on_fetch_user_join_server_info_finished()
{
	CraftBoxUserJoinServerInfos* inst = CraftBoxUserJoinServerInfos::instance();
	auto &join_server_infos = inst->join_server_infos();
	std::wstring sid = inst->select_sid();

	DuiLib::CListLabelElementUI* pListSelectElement = nullptr;

	craft_servers_cb_->RemoveAll();
	craft_players_cb_->RemoveAll();

	int i = 0;
	for (auto it = join_server_infos.begin(); it != join_server_infos.end(); ++it)
	{
		DuiLib::CListLabelElementUI* pListElement = nullptr;
		if (!default_list_label_builder_.GetMarkup()->IsValid())
		{
			pListElement = static_cast<DuiLib::CListLabelElementUI*>(default_list_label_builder_.Create(_T("default_listlabelelement.xml"), (UINT)0, nullptr, GetPaintManager()));
		}
		else
		{
			pListElement = static_cast<DuiLib::CListLabelElementUI*>(default_list_label_builder_.Create((UINT)0, GetPaintManager()));
		}
		DCHECK(pListElement);

		pListElement->SetText(it->second->server_name.c_str());
		pListElement->AddCustomAttribute(L"sid", it->second->sid.c_str());
		craft_servers_cb_->AddAt(pListElement, i++);

		if (!sid.empty() && it->second->sid == sid)
		{
			pListSelectElement = pListElement;
			craft_servers_cb_->SelectItem(i - 1);
		}
	}

	if (pListSelectElement == nullptr)
	{
		if (0 < i)
			craft_servers_cb_->SelectItem(0);
	}
}
