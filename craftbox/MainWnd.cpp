#include "MainWnd.h"
#include "resource.h"
#include "MenuWnd.h"
#include "glog/logging.h"
#include "minecraft_common/MinecraftCommon.h"
#include "minecraft_common/MinecraftDownload.h"
#include "MinecraftLauncher.h"
#include "base/PathService.h"
#include "base/file_version.h"
#include "LocalGameRecordPersistence.h"
#include "MinecraftDownloadRecordPersistence.h"
#include "JavaRecordPersistence.h"
#include "LocalGameComboMenu.h"
#include "ConfigWindow.h"
#include "JavaEnvironment.h"
#include "minecraft_common/JavaFinder.h"
#include "DownloadConfirmWnd.h"
#include "ShellNotifyIcon.h"
#include "minecraft_common/JavaDownload.h"
#include "base/system_info.h"
#include "MinecraftUserIdentityFactory.h"
#include "YggdrasilPersistence.h"
#include "LoadingWnd.h"
#include "LocalGameListContainerElementUI.h"
#include "UserAcountPersistence.h"
#include "BoxUserInfoManager.h"
#include "ModifyNoteWnd.h"
#include "ui_helper.h"
#include "OfficialMinecraftDownloadItemController.h"
#include "IntergrationDownloadItemController.h"
#include "base/HttpDownloadToFileWithCache.h"
#include "IntergrationVersionManager.h"
#include "base/StringHelper.h"
#include "base/SystemHelper.h"
#include "QuitConfirmWnd.h"
#include "UpdateSuccessNotifyWnd.h"
#include "base/url_encode.h"
#include "IntergrationDownloadRecordPersistence.h"
#include "minecraft_common/MinecraftVersionDetecter.h"
#include "WindowFinder.h"
#include "base/ResourceHelper.h"
#include "base/at_exit_manager.h"
#include "minecraft_common/ZipCmdProxy.h"
#include "GameCrashReportWnd.h"
#include "ServerConfig.h"
#include "CraftBoxUserJoinServerInfos.h"
#include "DefaultServerList.h"

#include <atlstr.h>
#include <atlconv.h>
#include <Shlobj.h>
#include <codecvt>
#include <WinInet.h>

#define REFRESH_DOWNLOADING_TIMER	(1001)

#define WM_NOTIFY_ICON	(WM_USER+1987)

MainWnd::MainWnd()
{
	global_ = this;
	mc_inst_manager_ = MinecraftInstanceManager::GetInstance();
	mc_inst_manager_->instances_added.connect(this, &MainWnd::on_mc_instances_added);
	mc_inst_manager_->instances_removed.connect(this, &MainWnd::on_mc_instances_removed);
	mc_inst_manager_->instances_all_removed.connect(this, &MainWnd::on_mc_instances_all_revoced);
	mc_inst_manager_->download_inst_added.connect(this, &MainWnd::on_official_downloading_instance_added);

	IntergrationVersionManager::global()->download_inst_added.connect(this, &MainWnd::on_intergration_downloading_instance_added);

	official_mc_vers_ = std::make_shared<OfficialMinecraftVersions>();
	official_mc_vers_->update_finish.connect(this, &MainWnd::on_official_game_list_updated);

	integrate_mc_vers_ = std::make_shared<IntegrationMinecraftVersions>();
	integrate_mc_vers_->update_finish.connect(this, &MainWnd::on_integrate_game_list_updated);

	auto java_env = JavaEnvironment::GetInstance();
	java_env->init();

	auto java_download = java_env->download_java();
	java_download->download_java_failed.connect(this, &MainWnd::on_java_download_failed);
	java_download->download_java_success.connect(this, &MainWnd::on_java_download_success);

	auto box_user_info_notify = BoxUserInfoManager::global();
	box_user_info_notify->fetching_user_info.connect(this, &MainWnd::on_fetching_user_info);
	box_user_info_notify->fetch_user_info_finished.connect(this, &MainWnd::on_fetch_user_info_finished);
	box_user_info_notify->logouted.connect(this, &MainWnd::on_logout);

	auto minecraft_process_mng = MinecraftProcessInfoManager::global();
	minecraft_process_mng->process_crash.connect(this, &MainWnd::on_game_crashed);
	minecraft_process_mng->process_may_crash.connect(this, &MainWnd::on_game_crashed);
}


MainWnd::~MainWnd()
{
	if (!!mc_inst_search_)
		mc_inst_search_->stop_search();

	save_all_data();

	global_ = nullptr;
}

void MainWnd::OnFinalMessage(HWND wnd)
{
	__super::OnFinalMessage(wnd);
	::PostQuitMessage(0);
}

DuiLib::CDuiString MainWnd::GetSkinFile()
{
	return L"ui.xml";
}

DuiLib::CDuiString MainWnd::GetSkinFolder()
{
	return L"skin\\";
}

LPCTSTR MainWnd::GetWindowClassName(void) const
{
	return L"zuimc_mainwnd";
}

LPCTSTR MainWnd::GetItemText(CControlUI* pControl, int iIndex, int iSubItem)
{
	CListTextElementUI* pListElement = (CListTextElementUI*)pControl->GetInterface(DUI_CTR_LISTTEXTELEMENT);
	if (pListElement)
	{
		return pListElement->GetText(iSubItem);
	}

	return L"";
}

void MainWnd::InitWindow()
{
	typedef BOOL
	(WINAPI *PChangeWindowMessageFilterEx)(
		__in HWND hwnd,                                         // Window
		__in UINT message,                                      // WM_ message
		__in DWORD action,                                      // Message filter action value
		__inout_opt PCHANGEFILTERSTRUCT pChangeFilterStruct);
	PChangeWindowMessageFilterEx func = (PChangeWindowMessageFilterEx)GetProcAddress(GetModuleHandleW(L"User32.dll"), "ChangeWindowMessageFilterEx");
	if (func)
	{
		if (0 == func(this->m_hWnd, WM_COPYDATA, MSGFLT_ALLOW, nullptr))
		{
			LOG(ERROR) << "ChangeWindowMessageFilterEx failed ";
		}
	}

	login_btn_ = static_cast<CButtonUI*>(m_PaintManager.FindControl(L"login"));
	user_header_btn_ = static_cast<CButtonUI*>(m_PaintManager.FindControl(L"user_head"));
	main_page_ = static_cast<CTabLayoutUI*>(m_PaintManager.FindControl(L"pages"));
	game_list_ = static_cast<CTabLayoutUI*>(m_PaintManager.FindControl(L"game_list"));
	local_game_list_ = static_cast<CListUI*>(m_PaintManager.FindControl(_T("local_game_list")));
	official_game_list_ = static_cast<CListUI*>(m_PaintManager.FindControl(L"official_game_list"));
	integrate_game_list_ = static_cast<CListUI*>(m_PaintManager.FindControl(L"intergrate_game_list"));
	CHECK(user_header_btn_ && login_btn_ && main_page_ && local_game_list_ && official_game_list_ && game_list_ && integrate_game_list_);

	main_page_opt_ = static_cast<COptionUI*>(m_PaintManager.FindControl(L"main_page"));
	client_page_opt_ = static_cast<COptionUI*>(m_PaintManager.FindControl(L"client_page"));
	server_page_opt_ = static_cast<COptionUI*>(m_PaintManager.FindControl(L"server_page"));
	download_page_opt_ = static_cast<COptionUI*>(m_PaintManager.FindControl(L"download_page"));
	downloading_ani_ = static_cast<CGifAnimUI*>(m_PaintManager.FindControl(L"downloading_ani"));

	backward_btn_ = static_cast<CButtonUI*>(m_PaintManager.FindControl(L"backward_btn"));
	forward_btn_ = static_cast<CButtonUI*>(m_PaintManager.FindControl(L"forward_btn"));
	refresh_btn_ = static_cast<CButtonUI*>(m_PaintManager.FindControl(L"refresh_btn"));
	wb_navigator_layout_ = static_cast<CHorizontalLayoutUI*>(m_PaintManager.FindControl(L"webnavigator_layout"));
	CHECK(backward_btn_ && forward_btn_ && refresh_btn_ && wb_navigator_layout_);

	setting_btn_ = static_cast<CButtonUI*>(m_PaintManager.FindControl(L"setting"));
	min_btn_ = static_cast<CButtonUI*>(m_PaintManager.FindControl(L"min"));
	close_btn_ = static_cast<CButtonUI*>(m_PaintManager.FindControl(L"close"));
	official_game_tab_underground_ = static_cast<CButtonUI*>(m_PaintManager.FindControl(L"official_game_tab_underground"));
	local_game_tab_underground_ = static_cast<CLabelUI*>(m_PaintManager.FindControl(L"local_game_tab_underground"));
	official_game_tab_btn_ = static_cast<CButtonUI*>(m_PaintManager.FindControl(L"official_game_tab_button"));
	local_game_tab_btn_ = static_cast<CButtonUI*>(m_PaintManager.FindControl(L"local_game_tab_button"));
	intergrate_game_tab_btn_ = static_cast<CButtonUI*>(m_PaintManager.FindControl(L"intergrate_game_tab_button"));
	intergrate_game_tab_underground_ = static_cast<CLabelUI*>(m_PaintManager.FindControl(L"intergrate_game_tab_underground"));

	
	search_game_btn_ = static_cast<CButtonUI*>(m_PaintManager.FindControl(L"searchgame"));
	search_tips_label_ = static_cast<CLabelUI*>(m_PaintManager.FindControl(L"search_game_tips"));
	search_ani_ = static_cast<CGifAnimUI*>(m_PaintManager.FindControl(L"search_game_ani"));
	download_tips_label_ = static_cast<CLabelUI*>(m_PaintManager.FindControl(L"download_game_tips"));
	external_dl_btn_ = static_cast<CButtonUI*>(m_PaintManager.FindControl(L"external_dl_btn"));
	only_show_release_type_ = static_cast<COptionUI*>(m_PaintManager.FindControl(L"only_show_release_type"));
	select_game_btn_ = static_cast<CButtonUI*>(m_PaintManager.FindControl(L"select_game_btn"));
	
	launch_game_btn_ = static_cast<CButtonUI*>(m_PaintManager.FindControl(L"game_launcher"));
	CHECK(external_dl_btn_ && downloading_ani_ && search_ani_ && intergrate_game_tab_btn_ && intergrate_game_tab_underground_ && only_show_release_type_ && download_tips_label_ && main_page_opt_ && client_page_opt_ && server_page_opt_ && download_page_opt_  && select_game_btn_ && launch_game_btn_ && search_tips_label_ && search_game_btn_ && min_btn_ && setting_btn_ && close_btn_ && official_game_tab_btn_ && local_game_tab_btn_ && official_game_tab_underground_ && local_game_tab_underground_);

	downloading_game_tab_btn_ = static_cast<CButtonUI*>(m_PaintManager.FindControl(L"downloading_tab_button"));
	downloaded_game_tab_btn_ = static_cast<CButtonUI*>(m_PaintManager.FindControl(L"downloaded_tab_button"));
	downloading_tab_underground_ = static_cast<CLabelUI*>(m_PaintManager.FindControl(L"downloading_tab_underground"));
	downloaded_tab_underground_ = static_cast<CLabelUI*>(m_PaintManager.FindControl(L"downloaded_tab_underground"));
	download_tab_layout_ = static_cast<CTabLayoutUI*>(m_PaintManager.FindControl(L"download_tab_layout"));
	downloading_list_ = static_cast<CListUI*>(m_PaintManager.FindControl(L"donwloading_list"));
	downloaded_list_ = static_cast<CListUI*>(m_PaintManager.FindControl(L"downloaded_list"));

	main_page_wb_ = static_cast<CWebBrowserUI*>(m_PaintManager.FindControl(L"main_page_wb"));
	server_page_wb_ = static_cast<CWebBrowserUI*>(m_PaintManager.FindControl(L"server_page_wb"));
	user_page_wb_ = static_cast<CWebBrowserUI*>(m_PaintManager.FindControl(L"user_page_wb"));
	
	feedback_btn_ = static_cast<CButtonUI*>(m_PaintManager.FindControl(L"feedback"));
	product_version_label = static_cast<CLabelUI*>(m_PaintManager.FindControl(L"product_version"));

	search_server_edit_ = static_cast<CEditUI*>(m_PaintManager.FindControl(L"search_server_edit"));
	search_server_btn_ = static_cast<CButtonUI*>(m_PaintManager.FindControl(L"search_server_btn"));
	CHECK(search_server_btn_ && search_server_edit_);

	CHECK(feedback_btn_
		&& product_version_label
		&& main_page_wb_
		&& server_page_wb_
		&& user_page_wb_
		&& downloaded_game_tab_btn_
		&& downloaded_game_tab_btn_
		&& downloading_tab_underground_
		&& downloaded_tab_underground_
		&& download_tab_layout_
		&& downloading_list_
		&& downloaded_list_
		);

	downloading_ani_->SetVisible(false);

	search_ani_->StopGif();
	search_ani_->SetVisible(false);

	only_show_release_type_->SetEnabled(false);

	main_page_wb_->SetWebBrowserEventHandler(main_page_event_handler_.get());
	server_page_wb_->SetWebBrowserEventHandler(server_page_event_handler_.get());
	user_page_wb_->SetWebBrowserEventHandler(user_center_event_handler_.get());

	std::list<CraftBoxPageEventHandler*> items{ main_page_event_handler_.get(), server_page_event_handler_.get(), user_center_event_handler_.get() };
	for (auto item : items)
	{
		item->launch_server.connect(this, &MainWnd::launch_server);
		item->login_success.connect(this, &MainWnd::on_login_success);
		item->modify_header.connect(this, &MainWnd::on_modify_header);
		item->need_update_pids.connect(CraftBoxUserJoinServerInfos::instance(), &CraftBoxUserJoinServerInfos::update_craftbox_user_join_server_infos);
		item->navigator_command_state_changed.connect(this, &MainWnd::on_wb_command_state_changed);
		item->download_game.connect(this, &MainWnd::on_download_server_integrate_pack);
		item->new_web_page.connect(this, &MainWnd::on_new_web_page);
	}

	user_header_btn_->SetHotBorderColor(0xFF73E2FF);
	default_header_pic_ = user_header_btn_->GetBkImage();
	std::weak_ptr<MainWnd> weak_obj = this->shared_from_this();
	AsyncTaskDispatch::current()->post_task(make_closure([weak_obj]()
	{
		auto obj = weak_obj.lock();
		if (!!obj && ::IsWindow(obj->GetHWND()))
		{
			obj->user_page_wb_->SetDelayCreate(false);
			obj->main_page_wb_->SetDelayCreate(false);
			obj->server_page_wb_->SetDelayCreate(false);

			obj->user_page_wb_->Navigate2(L"about:blank");
			obj->main_page_wb_->Navigate2(L"about:blank");
			obj->server_page_wb_->Navigate2(L"about:blank");

			uint32_t sid = ServerConfig::global()->sid();
			if (sid != 0)
			{
				std::wstring url = L"http://user.zuimc.com/sid/";
				url += std::to_wstring(sid);
				obj->main_page_wb_->Navigate2(url.c_str());
			}
			else
				obj->main_page_wb_->Navigate2(mcfe::g_main_page);

		}
		
	}), true);

	USES_CONVERSION;
	product_version_label->SetText(ATL::CA2W(FileVersion::app_file_version().c_str()));

	main_page_opt_->Selected(true);

	ui_select_local_game_tab();
	ui_select_downloading_game_tab();

	official_mc_vers_->start_update_lists();
	integrate_mc_vers_->start_update_lists();

	{
		std::weak_ptr<MainWnd> weak_obj = this->shared_from_this();
		std::thread t([weak_obj]()
		{
			YggdrasilPersistence persistence;
			std::string access_token, client_token;
			persistence.load(access_token, client_token);
			AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj, access_token, client_token]()
			{
				auto obj = weak_obj.lock();
				if (!!obj)
				{
					obj->on_load_yggdraisl_account_finished(access_token, client_token);
				}
			}), true);
			
		});
		yggdrasil_info_load_thread_ = std::move(t);
	}

	{
		std::weak_ptr<MainWnd> weak_obj = this->shared_from_this();
		std::thread t([weak_obj]()
		{
			LocalGameRecordPersistence persistence;
			persistence.load();
			
			AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj]()
			{
				auto obj = weak_obj.lock();
				if (!!obj)
				{
					obj->on_local_game_record_load_finished();
				}
			}), true);
		});
		local_game_record_load_thread_ = std::move(t);
	}
	
	{
		std::thread t([]()
		{
			MinecraftDownloadRecordPersistence persistence;
			persistence.load();
		});
		download_record_load_thread_ = std::move(t);
	}

	{
		std::thread t([]()
		{
			IntergrationDownloadRecordPersistence persistence;
			persistence.load();
		});
		integration_game_record_load_thread_ = std::move(t);
	}

	{
		std::weak_ptr<MainWnd> weak_obj = this->shared_from_this();
		std::thread t([weak_obj]()
		{
			JavaRecordPersistence persistence;
			persistence.load();

			AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj]()
			{
				auto obj = weak_obj.lock();
				if (!!obj)
				{
					obj->on_java_record_load_finished();
				}
			}), true);

		});
		java_record_load_thread_ = std::move(t);
	}
	
	AsyncTaskDispatch::current()->post_delay_task(make_closure([weak_obj]()
	{
		auto obj = weak_obj.lock();
		if (!!obj)
		{
			HICON hicon = (HICON)GetClassLongPtrW(obj->m_hWnd, GCLP_HICONSM);
			if (hicon == nullptr)
			{
				hicon = (HICON)::SendMessage(obj->GetHWND(), WM_GETICON, ICON_SMALL2, 0);
			}
			DCHECK(hicon != nullptr);
			obj->notify_icon_ = std::make_shared<ShellNotifyIcon>(obj->GetHWND(), WM_NOTIFY_ICON, hicon);
			obj->notify_icon_->show_notify_icon();
		}
	}), true, 500);

	DefaultServerList::instance()->fetch_server_list();


	delay_save_all_data();	
}

DuiLib::UILIB_RESOURCETYPE MainWnd::GetResourceType() const
{
#ifdef _DEBUG
	return UILIB_FILE;
#else
	return UILIB_ZIPRESOURCE;
#endif
}

LPCWSTR MainWnd::GetResourceID() const
{
	return MAKEINTRESOURCEW(IDR_ZIPRES_SKIN);
}

void MainWnd::OnClick(TNotifyUI& msg)
{
	if (msg.pSender == close_btn_)
	{
		Close(0);
	}
	else if (msg.pSender == min_btn_)
	{
		SendMessage(WM_SYSCOMMAND, SC_MINIMIZE, 0);
	}
	else if (msg.pSender == setting_btn_)
	{
		show_config_wnd(ConfigWindow::kGameSetting, true);
	}
	else if (msg.pSender == login_btn_ || msg.pSender == user_header_btn_)
	{
		show_user_center_page();
	}
	else if (msg.pSender == official_game_tab_btn_)
	{
		ui_select_official_game_tab();
		game_list_->SelectItem(2);
	}
	else if (msg.pSender == local_game_tab_btn_)
	{
		ui_select_local_game_tab();
		game_list_->SelectItem(0);
	}
	else if (msg.pSender == intergrate_game_tab_btn_)
	{
		ui_select_intergrate_game_tab();
		game_list_->SelectItem(1);
	}
	else if (msg.pSender == search_game_btn_)
	{
		if (!mc_inst_search_)
			init_mc_instance_search_obj();

		if (mc_inst_search_->is_searching())
		{
			mc_inst_search_->stop_search();
		}
		else
		{
			MenuWnd* menu = new MenuWnd();
			auto rect = search_game_btn_->GetPos();
			POINT pt = { rect.left - 4, rect.bottom - 4 };
			::ClientToScreen(*this, &pt);
			menu->Init(msg.pSender, pt, L"AddLocalGameBtnMenu.xml");
			menu->AdjustPostion();
		}
	}
	else if (msg.pSender == select_game_btn_)
	{
		auto* menu = new LocalGameComboMenu();
		menu->Init(select_game_btn_, { 0, 0 }, L"Local_game_combo_list.xml");
		HWND menu_wnd = menu->GetHWND();
		RECT menu_rect;
		auto x = select_game_btn_->GetX();
		auto y = select_game_btn_->GetY();
		POINT pt = { x, y };
		ClientToScreen(m_hWnd, &pt);
		x = pt.x;
		y = pt.y;
		GetWindowRect(menu_wnd, &menu_rect);
		::MoveWindow(menu_wnd, x - 4, y - (menu_rect.bottom - menu_rect.top) + 4, select_game_btn_->GetWidth() + 8, menu_rect.bottom - menu_rect.top, FALSE);
		menu->AdjustPostion();
	}
	else if (msg.pSender == backward_btn_)
	{
		IWebBrowser2* handler = (IWebBrowser2*)msg.pSender->GetTag();
		if (handler)
		{
			handler->GoBack();
		}
	}
	else if (msg.pSender == forward_btn_)
	{
		IWebBrowser2* handler = (IWebBrowser2*)msg.pSender->GetTag();
		if (handler)
		{
			handler->GoForward();
		}
	}
	else if (msg.pSender == refresh_btn_)
	{
		IWebBrowser2* handler = (IWebBrowser2*)msg.pSender->GetTag();
		if (handler)
		{
			CComVariant var(REFRESH_NORMAL);
			handler->Refresh2(&var);
		}
	}
	else if (msg.pSender == search_server_btn_)
	{
		std::wstring search_text = search_server_edit_->GetText().GetData();
		if (!search_text.empty())
		{
			std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
			std::string url = converter.to_bytes(search_text);
			url = UrlEncode(url);
			url = "http://user.zuimc.com/boxsearch?word=" + url;
			server_page_opt_->Selected(true);
			USES_CONVERSION;
			server_page_wb_->Navigate2(ATL::CA2W(url.c_str()));
		}
		
	}
	else if (msg.pSender == external_dl_btn_)
	{
		::ShellExecuteW(this->GetHWND(), L"open", L"http://pan.baidu.com/s/1i56lDHj", nullptr, nullptr, SW_SHOW);
	}
	else if (feedback_btn_ == msg.pSender)
	{
		::ShellExecuteA(this->GetHWND(), "open", mcfe::g_feedback_url, nullptr, nullptr, SW_SHOW);
	}
	else if (downloading_game_tab_btn_ == msg.pSender)
	{
		ui_select_downloading_game_tab();
		download_tab_layout_->SelectItem(0, false);
	}
	else if (msg.pSender == downloaded_game_tab_btn_)
	{
		ui_select_downloaded_game_tab();
		download_tab_layout_->SelectItem(1, false);
	}
	else if (msg.pSender == launch_game_btn_)
	{
		auto ver_str = select_game_btn_->GetCustomAttribute(L"version_str");
		auto base_dir = select_game_btn_->GetCustomAttribute(L"base_dir");
		if (ver_str == nullptr || base_dir == nullptr)
		{
			MessageBoxW(this->GetHWND(), L"你需要先添加本地游戏,或者直接使用启动器的下载游戏功能", L"找不到游戏哇 ╮(╯▽╰)╭", MB_OK | MB_ICONINFORMATION);
			game_list_->SelectItem(1);
			ui_select_intergrate_game_tab();
			client_page_opt_->Selected(true);
		}
		else
			AsyncTaskDispatch::current()->post_task(make_closure(&MainWnd::launch_game, this->shared_from_this(), base_dir, ver_str), true);
	}
	else if (msg.pSender->GetName().Compare(L"open_dir") == 0)
	{
		auto base_dir = msg.pSender->GetCustomAttribute(L"base_dir");
		auto version_dir = msg.pSender->GetCustomAttribute(L"version_dir");
		std::wstring dir = base_dir;
		dir += L"\\versions\\";
		dir += version_dir;
		::ShellExecuteW(this->GetHWND(), L"open", dir.c_str(), nullptr, nullptr, SW_SHOW);

	}
	else if (msg.pSender->GetName().Compare(L"remove") == 0)
	{
		auto parent = msg.pSender->GetParent();
		while (parent && wcscmp(parent->GetClass(), L"LocalGameListContainerElement") != 0)
			parent = parent->GetParent();
		DCHECK(parent);
		if (parent)
		{
			std::wstring base_dir = msg.pSender->GetCustomAttribute(L"base_dir");
			std::wstring version_dir = msg.pSender->GetCustomAttribute(L"version_dir");
			local_game_list_->Remove(parent, true);
			mc_inst_manager_->remove_minecraft_instance(base_dir, version_dir);
		}
	}
	else if (msg.pSender->GetName().Compare(L"note") == 0)
	{
		auto ctrl = msg.pSender->GetParent();
		while (ctrl && ctrl->GetClass() != L"LocalGameListContainerElement")
			ctrl = ctrl->GetParent();

		if (ctrl)
		{
			CLabelUI* name = static_cast<CLabelUI*>(m_PaintManager.FindSubControlByName(ctrl, L"name"));
			if (name)
			{
				auto base_dir = name->GetCustomAttribute(L"base_dir");
				auto version_str = name->GetCustomAttribute(L"version_dir");
				auto note = name->GetCustomAttribute(L"note");

				ModifyNoteWnd modify_note_wnd;
				modify_note_wnd.set_base_dir(base_dir);
				modify_note_wnd.set_version_str(version_str);
				modify_note_wnd.set_old_note(note ? note : L"");

				if (0 != modify_note_wnd.Create(this->GetHWND(), NULL, WS_VISIBLE | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_TOOLWINDOW))
				{
					modify_note_wnd.CenterWindow();
					modify_note_wnd.ShowModal();

					std::wstring new_node = modify_note_wnd.get_note();
					if (new_node.compare(note ? note : L"") != 0)
					{
						std::wstring new_text = get_ui_text_for_mc_inst(version_str, new_node);

						if (new_node.empty())
							name->RemoveCustomAttribute(L"note");
						else
							name->AddCustomAttribute(L"note", new_node.c_str());
						name->SetText(new_text.c_str());
						ctrl->NeedUpdate();
						MinecraftInstanceManager::MinecraftInstanceItem item;
						item.base_dir = base_dir;
						item.version_str = version_str;
						item.note = new_node;
						MinecraftInstanceManager::GetInstance()->modify_minecraft_instance_info(item);
					}
				}
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
	else if (msg.pSender->GetName().Compare(L"download") == 0)
	{
		auto dl_type = msg.pSender->GetCustomAttribute(L"download_type");
		if (dl_type == nullptr)
		{
			DCHECK(0);
			return;
		}

		if (wcscmp(L"0", dl_type) == 0)
		{
			std::wstring version_str = msg.pSender->GetCustomAttribute(L"version");
			DownloadConfirmWnd dl_confirm_wnd;
			dl_confirm_wnd.set_title(L"下载 " + version_str);
			dl_confirm_wnd.Init(this->GetHWND());
			dl_confirm_wnd.CenterWindow();
			dl_confirm_wnd.ShowModal();
			std::wstring dl_path = dl_confirm_wnd.selected_download_path();
			if (!dl_path.empty())
			{
				auto *ctrl = msg.pSender;
				std::wstring url = ctrl->GetCustomAttribute(L"url");
				DCHECK(!version_str.empty() && !url.empty());
				std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
				bool exist_before = false;
				dl_path += L"\\.minecraft";
				auto dl_obj = mc_inst_manager_->download_mc_instance(dl_path, version_str, converter.to_bytes(url), &exist_before);
				dl_obj->start_download(true);

				if (!exist_before)
				{
					std::wstring msg = L"下载任务\"" + version_str + L"\"成功添加";
					notify_icon_->show_notify_message(msg.c_str(), mcfe::g_app_name, 3000);
				}
					
				else
				{
					std::wstring msg = L"下载任务\"" + version_str + L"\"已经存在";
					notify_icon_->show_notify_message(msg.c_str(), mcfe::g_app_name, 3000);
				}
			}
		}
		else if(wcscmp(L"1", dl_type) == 0)
		{
			std::wstring version_str = msg.pSender->GetCustomAttribute(L"version");
			DownloadConfirmWnd dl_confirm_wnd;
			dl_confirm_wnd.set_title(L"下载 " + version_str);
			dl_confirm_wnd.Init(this->GetHWND());
			dl_confirm_wnd.show_tips(false);
			dl_confirm_wnd.CenterWindow();
			dl_confirm_wnd.ShowModal();
			std::wstring dl_path = dl_confirm_wnd.selected_download_path();
			if (!dl_path.empty())
			{
				auto *ctrl = msg.pSender;
				std::wstring url = ctrl->GetCustomAttribute(L"url");
				std::wstring size_str = ctrl->GetCustomAttribute(L"size");
				std::wstring sha1_w = ctrl->GetCustomAttribute(L"sha1");
				std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
				std::string sha1 = converter.to_bytes(sha1_w);

				DCHECK(!version_str.empty() && !url.empty() && !size_str.empty() && !sha1.empty());

				uint64_t size = _wtoll(size_str.c_str());

				std::wstring guid_string;
				guid_wstring(guid_string);

				bool exist_before = false;
				auto dl_obj = IntergrationVersionManager::global()->download_instance(converter.to_bytes(url), dl_path + L"\\" + guid_string, sha1, size, dl_path + L"\\" + version_str, version_str, &exist_before);
				dl_obj->start_download();

				if (!exist_before)
				{
					std::wstring msg = L"下载任务\"" + version_str + L"\"成功添加";
					notify_icon_->show_notify_message(msg.c_str(), mcfe::g_app_name, 3000);

					//on_intergration_downloading_instance_added(dl_obj, version_str, dl_path+L"\\"+version_str, size, sha1);
				}

				else
				{
					std::wstring msg = L"下载任务\"" + version_str + L"\"已经存在";
					notify_icon_->show_notify_message(msg.c_str(), mcfe::g_app_name, 3000);
				}
			}
		}
		
	}
	else if (msg.pSender->GetName().Compare(L"cancel_download") == 0)
	{
		DownloadingItemController *dl_obj = (DownloadingItemController*)msg.pSender->GetTag();
		if (dl_obj == nullptr)
		{
			DCHECK(0);
			return;
		}

		dl_obj->remove();
	}
	else if (msg.pSender->GetName() == L"pause_download")
	{
		DownloadingItemController *dl_obj = (DownloadingItemController*)msg.pSender->GetTag();
		if (dl_obj == nullptr)
		{
			DCHECK(0);
			return;
		}

		dl_obj->pause();
	}
	else if (msg.pSender->GetName() == L"start_download")
	{
		DownloadingItemController *dl_obj = (DownloadingItemController*)msg.pSender->GetTag();
		if (dl_obj == nullptr)
		{
			DCHECK(0);
			return;
		}

		dl_obj->start();
	}
	else if (msg.pSender->GetName() == L"retry_download")
	{
		DownloadingItemController *dl_obj = (DownloadingItemController*)msg.pSender->GetTag();
		if (dl_obj == nullptr)
		{
			DCHECK(0);
			return;
		}

		dl_obj->start();
	}
}

void MainWnd::on_official_game_list_updated(bool success)
{
	if (success)
	{
		only_show_release_type_->SetEnabled(true);
		only_show_release_type_->Selected(true);
	}
	else
	{
		AsyncTaskDispatch::main_thread()->post_delay_task(make_closure([this]()
		{
			official_mc_vers_->start_update_lists();
		}), true, 5000);

	}
}

void MainWnd::on_integrate_game_list_updated(bool success)
{
	if (success)
	{
		integrate_game_list_->RemoveAll();

		auto versions = integrate_mc_vers_->version_list();
		for (auto it = versions.begin(); it != versions.end(); ++it)
		{
			CListContainerElementUI* pListElement = nullptr;
			if (!integrate_game_list_builder_.GetMarkup()->IsValid())
			{
				pListElement = static_cast<CListContainerElementUI*>(integrate_game_list_builder_.Create(_T("intergrate_game_list.xml"), (UINT)0, nullptr, &m_PaintManager));
			}
			else
			{
				pListElement = static_cast<CListContainerElementUI*>(integrate_game_list_builder_.Create((UINT)0, &m_PaintManager));
			}
			DCHECK(pListElement);

			CLabelUI* name = static_cast<CButtonUI*>(m_PaintManager.FindSubControlByName(pListElement, L"name"));
			if (name)
			{
				name->SetShowHtml(true);
				name->SetMultiLine(true);
				std::wstring text = L"<c #333333>" + (*it)->name + L" </c><n><c #888888>" + (*it)->desc + L"</c>";
				name->SetText(text.c_str());
			}

			CButtonUI* dl_btn = static_cast<CButtonUI*>(m_PaintManager.FindSubControlByName(pListElement, L"download"));
			if (dl_btn)
			{
				USES_CONVERSION;
				dl_btn->AddCustomAttribute(L"download_type", L"1");
				dl_btn->AddCustomAttribute(L"url", ATL::CA2W((*it)->profile_url.c_str()));
				dl_btn->AddCustomAttribute(L"version", (*it)->name.c_str());
				dl_btn->AddCustomAttribute(L"size", std::to_wstring((*it)->size).c_str());

				std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
				dl_btn->AddCustomAttribute(L"sha1", converter.from_bytes((*it)->sha1).c_str());
			}

			integrate_game_list_->Add(pListElement);
		}
	}
	else
	{
		AsyncTaskDispatch::main_thread()->post_delay_task(make_closure([this]()
		{
			integrate_mc_vers_->start_update_lists();
		}), true, 5000);

	}
}

void MainWnd::on_mc_instances_added(std::shared_ptr<MinecraftInstanceManager::MinecraftInstanceItem> item)
{
	LocalGameListContainerElementUI* pListElement = nullptr;
	if (!local_game_list_builder_.GetMarkup()->IsValid())
	{
		pListElement = static_cast<LocalGameListContainerElementUI*>(local_game_list_builder_.Create(_T("local_game_list_item.xml"), (UINT)0, this, &m_PaintManager));
	}
	else
	{
		pListElement = static_cast<LocalGameListContainerElementUI*>(local_game_list_builder_.Create(this, &m_PaintManager));
	}
	DCHECK(pListElement);

	auto &base_dir = item->base_dir;
	auto &version_str = item->version_str;
	auto &note_text = item->note;
	std::wstring path = base_dir + L"\\versions\\" + version_str;
	std::wstring tooltip = L"路径: " + path;

	CLabelUI* name = static_cast<CLabelUI*>(m_PaintManager.FindSubControlByName(pListElement, L"name"));
	if (name)
	{
		name->SetShowHtml(true);
		name->SetText(get_ui_text_for_mc_inst(item).c_str());
		name->SetToolTip(tooltip.c_str());
		name->AddCustomAttribute(L"base_dir", base_dir.c_str());
		name->AddCustomAttribute(L"version_dir", version_str.c_str());
		name->AddCustomAttribute(L"note", note_text.c_str());
	}

	CLabelUI* path_label = static_cast<CLabelUI*>(m_PaintManager.FindSubControlByName(pListElement, L"path"));
	if (path_label)
	{
		path_label->SetText(path.c_str());
		path_label->SetToolTip(tooltip.c_str());
	}

	CButtonUI* note = static_cast<CButtonUI*>(m_PaintManager.FindSubControlByName(pListElement, L"note"));
	if (note)
	{
		pListElement->mouse_entered.connect(this, [note]() {note->SetVisible(true); });
		pListElement->mouse_leaved.connect(this, [note]() {note->SetVisible(false); });
		note->SetVisible(false);
	}

	const wchar_t* btn_names[] = { L"launch", L"repair", L"open_dir", L"remove" };
	for (auto &name : btn_names)
	{
		CButtonUI* btn = static_cast<CButtonUI*>(m_PaintManager.FindSubControlByName(pListElement, name));
		if (btn)
		{
			btn->AddCustomAttribute(L"base_dir", base_dir.c_str());
			btn->AddCustomAttribute(L"version_dir", version_str.c_str());
		}
	}

	local_game_list_->Add(pListElement);

	select_game_btn_->SetText(get_ui_text_for_mc_inst(item, false).c_str());
	select_game_btn_->AddCustomAttribute(L"base_dir", base_dir.c_str());
	select_game_btn_->AddCustomAttribute(L"version_str", version_str.c_str());
	select_game_btn_->SetToolTip(tooltip.c_str());
}

void MainWnd::on_mc_instances_removed(std::shared_ptr<MinecraftInstanceManager::MinecraftInstanceItem> item)
{
	auto text = select_game_btn_->GetText();
	if (!text.IsEmpty())
	{
		auto base_dir = select_game_btn_->GetCustomAttribute(L"base_dir");
		auto version_str = select_game_btn_->GetCustomAttribute(L"version_str");

		if (wcsicmp(base_dir, item->base_dir.c_str()) != 0
			|| wcsicmp(version_str, item->version_str.c_str()) != 0)
			return;

		auto mc_insts = mc_inst_manager_->local_instances();
		auto it = mc_insts.begin();
		if (it != mc_insts.end())
		{
			auto obj = it->second.begin();
			if (obj != it->second.end())
			{
				select_game_btn_->SetText(get_ui_text_for_mc_inst(obj->second, false).c_str());
				select_game_btn_->SetToolTip(std::wstring(L"路径: " + obj->second->base_dir + L"\\versions\\" + obj->second->version_str).c_str());
				select_game_btn_->AddCustomAttribute(L"base_dir", obj->second->base_dir.c_str());
				select_game_btn_->AddCustomAttribute(L"version_str", obj->second->version_str.c_str());
			}
			else
			{
				DCHECK(0);
			}
		}
		else
		{
			select_game_btn_->SetText(L"");
			select_game_btn_->SetToolTip(L"");
			select_game_btn_->RemoveCustomAttribute(L"base_dir");
			select_game_btn_->RemoveCustomAttribute(L"version_str");
		}
	}
}

void MainWnd::on_official_downloading_instance_added(std::shared_ptr<MinecraftDownload> dl_obj)
{
	CListContainerElementUI* pListElement = nullptr;
	if (!donwloading_list_builder_.GetMarkup()->IsValid())
	{
		pListElement = static_cast<CListContainerElementUI*>(donwloading_list_builder_.Create(_T("downloading_list_item.xml"), (UINT)0, nullptr, &m_PaintManager));
	}
	else
	{
		pListElement = static_cast<CListContainerElementUI*>(donwloading_list_builder_.Create((UINT)0, &m_PaintManager));
	}
	DCHECK(pListElement);

	std::shared_ptr<OfficialMinecraftDownloadItemController> official_dl_item_ctrl = std::make_shared<OfficialMinecraftDownloadItemController>(dl_obj);
	official_dl_item_ctrl->state_changed.connect(this, &MainWnd::on_download_item_state_changed);

	std::wstring path_text = dl_obj->base_dir() + L"\\versions\\" + dl_obj->version_str();
	path_text = L"保存路径: " + path_text;
	CLabelUI* name = static_cast<CLabelUI*>(m_PaintManager.FindSubControlByName(pListElement, L"name"));
	if (name)
	{
		name->SetText(dl_obj->version_str().c_str());
		name->SetToolTip(path_text.c_str());
	}

	std::shared_ptr<DownloadItemAndAssociateUI> info(new DownloadItemAndAssociateUI);
	info->item = official_dl_item_ctrl;
	info->version = dl_obj->version_str();
	CLabelUI* progress = static_cast<CLabelUI*>(m_PaintManager.FindSubControlByName(pListElement, L"progress"));
	if (progress)
	{
		info->progress_label = progress;
	}
	CButtonUI* cancel_button = static_cast<CButtonUI*>(m_PaintManager.FindSubControlByName(pListElement, L"cancel_download"));
	if (cancel_button)
	{
		cancel_button->SetTag((UINT_PTR)(DownloadingItemController*)official_dl_item_ctrl.get());
		info->cancel_btn = cancel_button;
	}
	CButtonUI* start_dl_button = static_cast<CButtonUI*>(m_PaintManager.FindSubControlByName(pListElement, L"start_download"));
	if (start_dl_button)
	{
		start_dl_button->SetTag((UINT_PTR)(DownloadingItemController*)official_dl_item_ctrl.get());
		info->start_btn = start_dl_button;
	}
	CButtonUI* pause_button = static_cast<CButtonUI*>(m_PaintManager.FindSubControlByName(pListElement, L"pause_download"));
	if (pause_button)
	{
		pause_button->SetTag((UINT_PTR)(DownloadingItemController*)official_dl_item_ctrl.get());
		info->pause_btn = pause_button;
	}
	CButtonUI* retry_button = static_cast<CButtonUI*>(m_PaintManager.FindSubControlByName(pListElement, L"retry_download"));
	if (retry_button)
	{
		retry_button->SetTag((UINT_PTR)(DownloadingItemController*)official_dl_item_ctrl.get());
		info->retry_btn = retry_button;
	}

	info->list_item = pListElement;

	dl_info_map_[(DownloadingItemController*)official_dl_item_ctrl.get()] = info;

	switch (info->item->state())
	{
	case DownloadingItemController::kPaused:
		info->retry_btn->SetVisible(false);
		info->start_btn->SetVisible(true);
		info->pause_btn->SetVisible(false);
		info->cancel_btn->SetVisible(true);
		info->progress_label->SetText(L"已暂停");
		break;
	case DownloadingItemController::kDownloading:
		info->retry_btn->SetVisible(false);
		info->start_btn->SetVisible(false);
		info->pause_btn->SetVisible(true);
		info->cancel_btn->SetVisible(true);
		if (info->progress_label->GetText().IsEmpty())
			info->progress_label->SetText(L"计算中...");
		break;
	default:
		DCHECK(0);
	}

	downloading_list_->Add(pListElement);

	if (downloading_list_refresh_timer_ == 0)
	{
		downloading_list_refresh_timer_ = ::SetTimer(this->GetHWND(), REFRESH_DOWNLOADING_TIMER, 1000, nullptr);
	}
}

void MainWnd::on_mc_instances_search_finished(bool search_exe_dir_only)
{
	if (!this->IsWindow())
	{
		return;
	}

	mc_inst_search_->stop_search();
	search_tips_label_->SetText(L"");
	search_game_btn_->SetText(L"添加本地游戏");
	search_ani_->StopGif();
	search_ani_->SetVisible(false);

	//if (select_game_btn_->GetText().IsEmpty())
	{
		auto default_inst = MinecraftInstanceManager::GetInstance()->default_local_inst();
		if (!!default_inst)
		{
			select_game_btn_->SetText(get_ui_text_for_mc_inst(default_inst, false).c_str());
			select_game_btn_->AddCustomAttribute(L"base_dir", default_inst->base_dir.c_str());
			select_game_btn_->AddCustomAttribute(L"version_str", default_inst->version_str.c_str());
			std::wstring text = L"路径: " + default_inst->base_dir + L"\\versions\\" + default_inst->version_str;
			select_game_btn_->SetToolTip(text.c_str());
		}
		else if(search_exe_dir_only)
		{
			auto local_insts = MinecraftInstanceManager::GetInstance()->local_instances();
			auto it = local_insts.begin();
			if (it == local_insts.end())
			{
				std::weak_ptr<MainWnd> weak_obj = this->shared_from_this();
				AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj]()
				{
					auto obj = weak_obj.lock();
					if (!!obj && obj->IsWindow())
					{
						if (IDOK == ::MessageBoxW(obj->GetHWND(), L"没有发现游戏目录, 是否需要全盘搜索？", mcfe::g_app_name, MB_OKCANCEL | MB_ICONINFORMATION))
						{
							obj->ui_select_local_game_tab();
							obj->client_page_opt_->Selected(true);
							obj->mc_inst_search_->start_search(false);
						}
					}
				}), false);
			}
		}
	}
}

void MainWnd::on_mc_instances_search_begined()
{
	if (this->IsWindow())
	{
		search_tips_label_->SetText(L"扫描中...");
		search_game_btn_->SetText(L"停止扫描");

		search_ani_->PlayGif();
		search_ani_->SetVisible(search_game_btn_->IsVisible());
	}
	
}

void MainWnd::init_mc_instance_search_obj()
{
	mc_inst_search_ = std::make_shared<MinecraftInstanceSearch>();
	mc_inst_search_->search_canceled.connect(this, &MainWnd::on_mc_instances_search_finished);
	mc_inst_search_->search_finished.connect(this, &MainWnd::on_mc_instances_search_finished);
	mc_inst_search_->search_started.connect(this, &MainWnd::on_mc_instances_search_begined);
}

void MainWnd::manual_add_mc_directory()
{
	BROWSEINFOW bifolder;
	wchar_t FileName[MAX_PATH + 1] = { 0 };
	ZeroMemory(&bifolder, sizeof(BROWSEINFO));
	bifolder.hwndOwner = this->GetHWND();
	bifolder.pszDisplayName = FileName;
	bifolder.lpszTitle = L"请选择.minecraft文件夹所在路径";
	bifolder.ulFlags = BIF_USENEWUI | BIF_NONEWFOLDERBUTTON | BIF_BROWSEINCLUDEURLS | BIF_RETURNONLYFSDIRS;

	LPITEMIDLIST idl = SHBrowseForFolder(&bifolder);
	if (idl)
	{
		SHGetPathFromIDList(idl, FileName);
		wchar_t *ptr = StrStrIW(FileName, L"\\.minecraft");
		if (ptr)
		{
			ptr += wcslen(L"\\.minecraft");
			if (*ptr == 0 || *ptr == L'\\')
			{
				*ptr = 0;
				mc_inst_manager_->check_and_add_minecraft_instance(FileName);
				return;
			}
		}

		std::wstring dir = FileName;
		dir += L"\\.minecraft";
		if (::PathFileExistsW(dir.c_str()))
			mc_inst_manager_->check_and_add_minecraft_instance(dir.c_str());
	}
}

void MainWnd::on_java_download_failed()
{
	notify_icon_->show_notify_message(L"Java下载失败", mcfe::g_app_name);
}

void MainWnd::on_java_download_success()
{
	notify_icon_->show_notify_message(L"Java下载完成", mcfe::g_app_name);
}

void MainWnd::ui_select_local_game_tab()
{
	local_game_tab_underground_->SetAttribute(L"bkcolor", L"#FF3DA8CA");
	local_game_tab_btn_->SetAttribute(L"textcolor", L"#FF3DA8CA");

	official_game_tab_underground_->SetAttribute(L"bkcolor", L"#00000000");
	official_game_tab_btn_->SetAttribute(L"textcolor", L"#FF333333");

	intergrate_game_tab_btn_->SetAttribute(L"textcolor", L"#FF333333");
	intergrate_game_tab_underground_->SetAttribute(L"bkcolor", L"#00000000");

	search_game_btn_->SetVisible(true);
	search_tips_label_->SetVisible(true);
	search_ani_->SetVisible(!!mc_inst_search_?mc_inst_search_->is_searching():false);
	download_tips_label_->SetVisible(false);
	only_show_release_type_->SetVisible(false);
	external_dl_btn_->SetVisible(false);
}

void MainWnd::ui_select_official_game_tab()
{
	local_game_tab_underground_->SetAttribute(L"bkcolor", L"#00000000");
	local_game_tab_btn_->SetAttribute(L"textcolor", L"#FF333333");

	official_game_tab_underground_->SetAttribute(L"bkcolor", L"#FF3DA8CA");
	official_game_tab_btn_->SetAttribute(L"textcolor", L"FF3DA8CA");

	intergrate_game_tab_btn_->SetAttribute(L"textcolor", L"#FF333333");
	intergrate_game_tab_underground_->SetAttribute(L"bkcolor", L"#00000000");

	search_game_btn_->SetVisible(false);
	search_tips_label_->SetVisible(false);
	search_ani_->SetVisible(false);
	download_tips_label_->SetVisible(true);
	download_tips_label_->SetText(L"提示: 将多个版本的游戏下载到同一个目录, 可以减少重复下载的文件数量");
	only_show_release_type_->SetVisible(true);
	external_dl_btn_->SetVisible(false);
}

void MainWnd::ui_select_intergrate_game_tab()
{
	local_game_tab_underground_->SetAttribute(L"bkcolor", L"#00000000");
	local_game_tab_btn_->SetAttribute(L"textcolor", L"#FF333333");

	official_game_tab_underground_->SetAttribute(L"bkcolor", L"#00000000");
	official_game_tab_btn_->SetAttribute(L"textcolor", L"FF333333");

	intergrate_game_tab_btn_->SetAttribute(L"textcolor", L"#FF3DA8CA");
	intergrate_game_tab_underground_->SetAttribute(L"bkcolor", L"#FF3DA8CA");

	search_game_btn_->SetVisible(false);
	search_tips_label_->SetVisible(false);
	search_ani_->SetVisible(false);
	download_tips_label_->SetVisible(true);
	download_tips_label_->SetShowHtml(true);
	download_tips_label_->SetText(L"下载速度不够快? 你可以点击右边按钮通过百度网盘下载整合包。提取码: <c #FF0000>668h</c>");
	only_show_release_type_->SetVisible(false);
	external_dl_btn_->SetVisible(true);
}

void MainWnd::Notify(TNotifyUI& msg)
{
	if (msg.pSender == local_game_list_)
	{
		auto cur_sel = local_game_list_->GetCurSel();
		if (cur_sel == -1)
			return;

		if (msg.sType == L"itemselect")
		{
			int select_item = msg.wParam;
			DCHECK(cur_sel == select_item);
			
			auto item = local_game_list_->GetItemAt(select_item);
			if (item)
			{
				CLabelUI* name = static_cast<CLabelUI*>(m_PaintManager.FindSubControlByName(item, L"name"));
				if (name)
				{
					auto base_dir = name->GetCustomAttribute(L"base_dir");
					auto version_str = name->GetCustomAttribute(L"version_dir");
					auto note = name->GetCustomAttribute(L"note");
					
					select_game_btn_->AddCustomAttribute(L"base_dir", base_dir);
					select_game_btn_->AddCustomAttribute(L"version_str", version_str);

					std::wstring tooltip = L"路径: ";
					tooltip += base_dir;
					tooltip += L"\\versions\\";
					tooltip += version_str;

					if (note)
					{
						std::wstring text = note;
						text += L"(";
						text += version_str;
						text += L")";
						select_game_btn_->SetText(text.c_str());
					}
					else
						select_game_btn_->SetText(version_str);

					select_game_btn_->SetToolTip(tooltip.c_str());
				}
			}
		}
		if (msg.sType == _T("menu"))
		{
			MenuWnd* menu = new MenuWnd();
			POINT pt = { msg.ptMouse.x, msg.ptMouse.y };
			::ClientToScreen(*this, &pt);
			menu->Init(msg.pSender, pt, L"LocalGameListMenu.xml");
			menu->AdjustPostion();
		}
		else if (msg.sType == _T("open_dir"))
		{
			auto item = local_game_list_->GetItemAt(cur_sel);
			if (item)
			{
				CButtonUI* btn = static_cast<CButtonUI*>(m_PaintManager.FindSubControlByName(item, L"open_dir"));
				if (btn)
				{
					auto base_dir = btn->GetCustomAttribute(L"base_dir");
					auto version_dir = btn->GetCustomAttribute(L"version_dir");
					std::wstring dir = base_dir;
					dir += L"\\versions\\";
					dir += version_dir;
					::ShellExecuteW(this->GetHWND(), L"open", dir.c_str(), nullptr, nullptr, SW_SHOW);
				}
			}
		}
		else if (msg.sType == _T("open_mods_dir"))
		{
			auto item = local_game_list_->GetItemAt(cur_sel);
			if (item)
			{
				CButtonUI* btn = static_cast<CButtonUI*>(m_PaintManager.FindSubControlByName(item, L"open_dir"));
				if (btn)
				{
					auto base_dir = btn->GetCustomAttribute(L"base_dir");
					auto version_dir = btn->GetCustomAttribute(L"version_dir");

					std::shared_ptr<MinecraftProfile> profile = std::make_shared<MinecraftProfile>(base_dir, version_dir);
					if (!profile->load_game_profile_file(false))
					{
						::MessageBox(this->GetHWND(), L"不合法的游戏目录", mcfe::g_app_name, MB_OK | MB_ICONINFORMATION);
						return;
					}

					if (!profile->has_forge() && !profile->has_liteloader())
					{
						::MessageBox(this->GetHWND(), L"您选择的游戏没有安装Forge或者LiteLoader等模组API, 模组并不能使用。", mcfe::g_app_name, MB_OK | MB_ICONINFORMATION);
						return;
					}

					std::wstring dir = base_dir;
					dir += L"\\versions\\";
					dir += version_dir;
					dir += L"\\mods";
					::SHCreateDirectory(0, dir.c_str());
					::ShellExecuteW(this->GetHWND(), L"open", dir.c_str(), nullptr, nullptr, SW_SHOW);
				}
			}
		}
		else if (msg.sType == _T("open_res_dir"))
		{
			auto item = local_game_list_->GetItemAt(cur_sel);
			if (item)
			{
				CButtonUI* btn = static_cast<CButtonUI*>(m_PaintManager.FindSubControlByName(item, L"open_dir"));
				if (btn)
				{
					auto base_dir = btn->GetCustomAttribute(L"base_dir");
					auto version_dir = btn->GetCustomAttribute(L"version_dir");
					std::wstring dir = base_dir;
					dir += L"\\versions\\";
					dir += version_dir;
					dir += L"\\resourcepacks";
					::SHCreateDirectory(0, dir.c_str());
					::ShellExecuteW(this->GetHWND(), L"open", dir.c_str(), nullptr, nullptr, SW_SHOW);
				}
			}
		}
		else if (msg.sType == _T("open_shader_dir"))
		{
			auto item = local_game_list_->GetItemAt(cur_sel);
			if (item)
			{
				CButtonUI* btn = static_cast<CButtonUI*>(m_PaintManager.FindSubControlByName(item, L"open_dir"));
				if (btn)
				{
					auto base_dir = btn->GetCustomAttribute(L"base_dir");
					auto version_dir = btn->GetCustomAttribute(L"version_dir");

					std::shared_ptr<MinecraftProfile> profile = std::make_shared<MinecraftProfile>(base_dir, version_dir);
					if (!profile->load_game_profile_file(false))
					{
						::MessageBox(this->GetHWND(), L"不合法的游戏目录", mcfe::g_app_name, MB_OK | MB_ICONINFORMATION);
						return;
					}

					if (!profile->has_forge() && !profile->has_liteloader())
					{
						::MessageBox(this->GetHWND(), L"您选择的游戏没有安装Forge或者LiteLoader等模组API, 光影并不能使用。", mcfe::g_app_name, MB_OK | MB_ICONINFORMATION);
						return;
					}

					std::wstring dir = base_dir;
					dir += L"\\versions\\";
					dir += version_dir;
					dir += L"\\shaderpacks";
					::SHCreateDirectory(0, dir.c_str());
					::ShellExecuteW(this->GetHWND(), L"open", dir.c_str(), nullptr, nullptr, SW_SHOW);
				}
			}
		}
		else if (msg.sType == _T("open_saves_dir"))
		{
			auto item = local_game_list_->GetItemAt(cur_sel);
			if (item)
			{
				CButtonUI* btn = static_cast<CButtonUI*>(m_PaintManager.FindSubControlByName(item, L"open_dir"));
				if (btn)
				{
					auto base_dir = btn->GetCustomAttribute(L"base_dir");
					auto version_dir = btn->GetCustomAttribute(L"version_dir");
					std::wstring dir = base_dir;
					dir += L"\\versions\\";
					dir += version_dir;
					dir += L"\\saves";
					::SHCreateDirectory(0, dir.c_str());
					::ShellExecuteW(this->GetHWND(), L"open", dir.c_str(), nullptr, nullptr, SW_SHOW);
				}
			}
		}
		else if (msg.sType == _T("remove"))
		{
			auto item = local_game_list_->GetItemAt(local_game_list_->GetCurSel());
			if (item)
			{
				CButtonUI* btn = static_cast<CButtonUI*>(m_PaintManager.FindSubControlByName(item, L"remove"));
				if (btn)
				{
					std::wstring base_dir = btn->GetCustomAttribute(L"base_dir");
					std::wstring version_dir = btn->GetCustomAttribute(L"version_dir");
					local_game_list_->Remove(item, true);
					mc_inst_manager_->remove_minecraft_instance(base_dir, version_dir);
				}
			}
		}
		else if (msg.sType == _T("remove_all"))
		{
			local_game_list_->RemoveAll();
			mc_inst_manager_->remove_all_instance();
		}
	}
	else if (msg.pSender == client_page_opt_)
	{
		if (msg.sType.Compare(DUI_MSGTYPE_SELECTCHANGED) == 0)
		{
			main_page_->SelectItem(1);
			wb_navigator_layout_->SetVisible(false);
		}
	}
	else if (msg.pSender == main_page_opt_)
	{
		if (msg.sType.Compare(DUI_MSGTYPE_SELECTCHANGED) == 0)
		{
			main_page_->SelectItem(0);
			wb_navigator_layout_->SetVisible(true);
			on_wb_command_state_changed(main_page_event_handler_.get());
		}
	}
	else if (msg.pSender == server_page_opt_)
	{
		if (msg.sType.Compare(DUI_MSGTYPE_SELECTCHANGED) == 0)
		{
			if (first_show_server_page_)
			{
				first_show_server_page_ = false;
				server_page_wb_->Navigate2(mcfe::g_server_list_page);
			}
			main_page_->SelectItem(2);
			wb_navigator_layout_->SetVisible(true);
			on_wb_command_state_changed(server_page_event_handler_.get());
		}
	}
	else if (msg.pSender == download_page_opt_)
	{
		if (msg.sType.Compare(DUI_MSGTYPE_SELECTCHANGED) == 0)
		{
			main_page_->SelectItem(3);
			wb_navigator_layout_->SetVisible(false);
		}
	}
	else if (msg.pSender == search_game_btn_)
	{
		if (msg.sType == L"scan_local_game")
		{
			mc_inst_search_->start_search(false);
		}
		else if (msg.sType == L"manual_add_game_dir")
		{
			AsyncTaskDispatch::current()->post_task(make_closure(&MainWnd::manual_add_mc_directory, this->shared_from_this()), true);
		}
	}
	else if (msg.pSender == select_game_btn_)
	{
		if (msg.sType == L"itemclick")
		{
			auto element = (CListContainerElementUI*)msg.wParam;
			if (element)
			{
				auto version_str = element->GetCustomAttribute(L"version_str");
				auto base_dir = element->GetCustomAttribute(L"base_dir");
				auto note = element->GetCustomAttribute(L"note");
				std::wstring tooltip = L"路径: ";
				tooltip += base_dir;
				tooltip += L"\\versions\\";
				tooltip += version_str;

				if (note)
				{
					std::wstring text = note;
					text += L"(";
					text += version_str;
					text += L")";
					select_game_btn_->SetText(text.c_str());
				}
				else
					select_game_btn_->SetText(version_str);

				select_game_btn_->SetToolTip(tooltip.c_str());
				select_game_btn_->AddCustomAttribute(L"base_dir", base_dir);
				select_game_btn_->AddCustomAttribute(L"version_str", version_str);
			}

		}
	}
	else if (msg.pSender == only_show_release_type_)
	{
		if (msg.sType == L"selectchanged")
		{
			refresh_official_game_list(only_show_release_type_->IsSelected());
		}
	}
	
	__super::Notify(msg);
}

LRESULT MainWnd::HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
	if (uMsg == WM_TIMER)
	{
		return OnTimer(uMsg, wParam, lParam, bHandled);
	}
	else if (uMsg == WM_NOTIFY_ICON)
	{
		return OnNotifyIcon(uMsg, wParam, lParam, bHandled);
	}
	else if (uMsg == WM_QUERYENDSESSION)
	{
		bHandled = TRUE;
		return TRUE;
	}
	else if (uMsg == WM_ENDSESSION)
	{
		bHandled = TRUE;
		save_all_data();
		return 0;
	}
	else if (uMsg == WM_COPYDATA)
	{
		bHandled = TRUE;
		return OnCopyData(uMsg, wParam, lParam, bHandled);
	}
	return 0;
}

LRESULT MainWnd::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
	if (wParam == downloading_list_refresh_timer_)
	{
		bHandled = TRUE;
		ui_update_downloading_progress();
	}

	return 0;
}

LRESULT MainWnd::OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	POINT pt; pt.x = GET_X_LPARAM(lParam); pt.y = GET_Y_LPARAM(lParam);
	::ScreenToClient(*this, &pt);

	if (pt.x > 227 && pt.y > 75)
		return HTCLIENT;

	CControlUI* pControl = static_cast<CControlUI*>(m_PaintManager.FindControl(pt));
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

LRESULT MainWnd::ResponseDefaultKeyEvent(WPARAM wParam)
{
	return FALSE;
}

void MainWnd::show_notify_icon_menu(POINT *pt)
{
	MenuWnd *menu = new MenuWnd();
	if (menu->Init(0, *pt, L"shell_notify_icon_menu.xml"))
	{
		menu->AdjustNotifyIconPosition();
		menu->ShowWindow();
		menu->SetTopMost();
		::SetForegroundWindow(menu->GetHWND());
		menu->menu_item_clicked.connect(this, &MainWnd::on_notify_icon_menu_clicked);
	}
	
}

void MainWnd::ui_select_downloading_game_tab()
{
	downloading_game_tab_btn_->SetAttribute(L"textcolor", L"#FF3DA8CA");
	downloaded_game_tab_btn_->SetAttribute(L"textcolor", L"#FF333333");
	downloading_tab_underground_->SetAttribute(L"bkcolor", L"#FF3DA8CA");
	downloaded_tab_underground_->SetAttribute(L"bkcolor", L"#00000000");
}

void MainWnd::ui_select_downloaded_game_tab()
{
	downloading_game_tab_btn_->SetAttribute(L"textcolor", L"#FF333333");
	downloaded_game_tab_btn_->SetAttribute(L"textcolor", L"#FF3DA8CA");
	downloading_tab_underground_->SetAttribute(L"bkcolor", L"#00000000");
	downloaded_tab_underground_->SetAttribute(L"bkcolor", L"#FF3DA8CA");
}

LRESULT MainWnd::OnClose(UINT, WPARAM, LPARAM, BOOL& bHandled)
{
	if (need_really_quit_)
	{
		bHandled = false;
		on_really_close();
	}
	else
	{
		auto always_on_close = AppConfig::global()->get_value_for_key(L"always_on_close");
		auto on_close = AppConfig::global()->get_value_for_key(L"on_close");
		if (always_on_close == L"1")
		{
			if (on_close == L"2") //关闭
			{
				bHandled = false;
				on_really_close();
				return 0;
			}
			else if (on_close == L"1") //最小化
			{
				this->ShowWindow(false);
				return 0;
			}
		}
		
		std::weak_ptr<MainWnd> weak_obj = this->shared_from_this();
		AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj]()
		{
			auto obj = weak_obj.lock();
			if (!!obj)
			{
				obj->show_quit_confirm_window();
			}
		}), false);
	}

	return 0;
}

CControlUI * MainWnd::CreateControl(LPCTSTR pstrClass)
{
	if (pstrClass && wcscmp(pstrClass, L"LocalGameListContainerElement") == 0)
	{
		return new LocalGameListContainerElementUI();
	}

	return nullptr;
}

void MainWnd::save_all_data()
{
	auto java_env = JavaEnvironment::GetInstance();
	java_env->save();

	AppConfig::global()->save();

	if (local_game_record_load_thread_.joinable())
		local_game_record_load_thread_.join();

	{
		LocalGameRecordPersistence persistence;
		persistence.save();
	}

	if (download_record_load_thread_.joinable())
		download_record_load_thread_.join();

	{
		MinecraftDownloadRecordPersistence persistence;
		persistence.save();
	}

	if (yggdrasil_info_load_thread_.joinable())
		yggdrasil_info_load_thread_.join();

	{
		YggdrasilPersistence persistence;
		auto yggdrasil_auth = YggdrasilAuthentication::global();
		if (yggdrasil_auth->authenticate_state() == YggdrasilAuthentication::kValidated)
		{
			persistence.save(yggdrasil_auth->access_token(), yggdrasil_auth->client_token());
		}
		else
		{
			persistence.save("", "");
		}
		
	}

	if (integration_game_record_load_thread_.joinable())
		integration_game_record_load_thread_.join();
	
	{
		IntergrationDownloadRecordPersistence persistence;
		persistence.save();
	}

	if (java_record_load_thread_.joinable())
		java_record_load_thread_.join();

	{
		JavaRecordPersistence persistence;
		persistence.save();
	}
}

void MainWnd::ui_update_downloading_progress()
{
	bool show_downloading_ani = false;
	for (auto it = dl_info_map_.begin(); it != dl_info_map_.end(); ++it)
	{
		if (it->first->state() == DownloadingItemController::kDownloading)
		{
			show_downloading_ani = true;
			if (DownloadingItemController::kOfficial == it->first->type())
			{
				wchar_t buf[128] = { 0 };

				uint64_t downloaded_item_num = 0;
				uint64_t all_download_item_num = 0;
				it->first->get_download_progress(downloaded_item_num, all_download_item_num);

				wsprintf(buf, L"%I64u / %I64u  (文件数)", downloaded_item_num, all_download_item_num);
				it->second->progress_label->SetText(buf);
			}
			else
			{
				char buf[128] = { 0 };

				uint64_t downloaded_bytes = 0;
				uint64_t all_download_bytes = 0;
				it->first->get_download_progress(downloaded_bytes, all_download_bytes);
				double dledmb = ((double)downloaded_bytes) / 1024/1024;
				double allmb = ((double)all_download_bytes) / 1024 / 1024;
				sprintf(buf, "%.2lfMB / %.2lfMB", dledmb, allmb);

				USES_CONVERSION;
				it->second->progress_label->SetText(ATL::CA2W(buf));
			}
		}
	}

	if (show_downloading_ani != downloading_ani_->IsVisible())
	{
		downloading_ani_->SetVisible(show_downloading_ani);
		show_downloading_ani ? downloading_ani_->PlayGif() : downloading_ani_->StopGif();
	}
}

LRESULT MainWnd::OnNotifyIcon(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	switch (LOWORD(lParam))
	{
	case WM_RBUTTONUP:
	{
		bHandled = true;
		POINT pt;
		GetCursorPos(&pt);
		show_notify_icon_menu(&pt);
		break;
	}
		
	case WM_LBUTTONDBLCLK:
	{
		bHandled = true;
		this->ShowWindow();
		::SetForegroundWindow(this->GetHWND());
		break;
	}
	default:
		bHandled = false;
		break;
	}

	return 0;
}

void MainWnd::on_notify_icon_menu_clicked(DuiLib::TNotifyUI &msg)
{
	if (msg.pSender->GetName() == L"show_main_wnd")
	{
		this->ShowWindow();
		::SetForegroundWindow(this->GetHWND());
	}
	else if (msg.pSender->GetName() == L"quit_app")
	{
		need_really_quit_ = true;
		Close();
	}
}

void MainWnd::show_config_wnd(ConfigWindow::ConfigWndTab tab, bool async)
{
	if (!async)
	{
		if (!this->IsWindow())
			return;

		if (!config_wnd_)
		{
			config_wnd_ = std::make_shared<ConfigWindow>();
			config_wnd_->Init(this->GetHWND());
			config_wnd_->ui_select_tab(tab);

			config_wnd_->CenterWindow();
			config_wnd_->ShowModal();

			config_wnd_.reset();
		}
		else
			config_wnd_->ui_select_tab(tab);
	}
	else
	{
		AsyncTaskDispatch::main_thread()->post_task(make_closure(&MainWnd::show_config_wnd, this->shared_from_this(), tab, false), false);
	}
	
}

void MainWnd::on_login_success(bool auto_save)
{
	wchar_t cookies[2048] = { 0 };
	DWORD sz = _countof(cookies);
	BOOL ret = ::InternetGetCookieW(mcfe::g_user_page, nullptr, cookies, &sz);
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
	DCHECK(ret);
	if (ret == FALSE)
	{
		auto err = ::GetLastError();
		LOG(ERROR) << "get cookie failed with err:" << err;
	}
	if (auto_save)
	{
		if (ret)
		{
			save_cookie(converter.to_bytes(cookies));
		}
	}
	else
	{
		save_cookie("");
	}

	BoxUserInfoManager::global()->auth_with_cookie(converter.to_bytes(cookies));
	
} 

void MainWnd::launch_game(const std::wstring &base_dir, const std::wstring &version_str)
{
	std::shared_ptr<MinecraftProfile> profile = std::make_shared<MinecraftProfile>(base_dir, version_str);
	if (!::PathFileExistsW(profile->client_dir().c_str()) || !::PathIsDirectoryW(profile->client_dir().c_str()))
	{
		::MessageBoxW(this->GetHWND(), L"游戏路径不存在", L"启动失败", MB_ICONERROR | MB_OK);
		return;
	}

	if (profile->load_game_profile_file(true))
	{
		std::wstring java_path = JavaEnvironment::GetInstance()->selected_java_path();
		if (java_path.empty())
		{
			auto javas = JavaEnvironment::GetInstance()->java_paths_and_vers();
			auto it = javas.begin();
			if (it == javas.end())
			{
				::MessageBoxW(this->GetHWND(), L"请先配置Java路径", L"找不到Java", MB_OK | MB_ICONINFORMATION);
				show_config_wnd(ConfigWindow::kGameSetting, true);
				return;
			}
			else
			{
				java_path = it->first;
			}
		}

		auto *app_config = AppConfig::global();
		auto login_type = (mcfe::UserAccountType)_wtoi(app_config->get_value_for_key(L"account_type").c_str());
		std::shared_ptr<MinecraftUserIdentity> user_identity;
		std::wstring server;
		uint16_t port = 0;
		std::wstring server_name;
		std::string cmd;
		switch (login_type)
		{
		case mcfe::kOffline:
		{
			std::wstring offline_user_name = app_config->get_value_for_key(L"offline_player_name");
			if (offline_user_name.empty()/*|| !verify_offline_user_name(offline_user_name)*/)
			{
				::MessageBoxW(this->GetHWND(), L"离线模式的角色名字不能为空", L"请重新设置离线模式的角色名字", MB_OK | MB_ICONINFORMATION);
				show_config_wnd(ConfigWindow::kGameSetting, true);
				return;
			}
			user_identity = MinecraftUserIdentityFactory::create_user_identity_object(login_type);
			break;
		}

		case mcfe::kCraftBox:
		{
			if (!BoxUserInfoManager::global()->is_logined())
			{
				::MessageBoxW(this->GetHWND(), L"要使用创世神验证模式启动游戏, 您必须先登录", L"提示", MB_OK | MB_ICONINFORMATION);
				show_user_center_page();
				return;
			}

			auto join_server_infos = CraftBoxUserJoinServerInfos::instance()->join_server_infos();
			if (join_server_infos.empty())
			{
				::MessageBoxW(this->GetHWND(), L"在使用创世神验证模式时, 您需要先加入服务器才能开始游戏", L"提示", MB_OK | MB_ICONINFORMATION);
				server_page_opt_->Selected(true);
				return;
			}

			std::wstring sid = CraftBoxUserJoinServerInfos::instance()->select_sid();
			std::wstring pid = CraftBoxUserJoinServerInfos::instance()->select_pid();
			if (sid.empty() || pid.empty())
			{
				::MessageBoxW(this->GetHWND(), L"请先在选择服务器和角色名", L"提示", MB_OK | MB_ICONINFORMATION);
				show_config_wnd(ConfigWindow::kGameSetting, true);
				return;
			}

			auto it = join_server_infos.find(sid);
			if (it == join_server_infos.end())
			{
				DCHECK(0);
				return;
			}

			server = it->second->host;
			port = it->second->port;
			server_name = it->second->server_name;
			std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
			std::string pid_str = converter.to_bytes(pid);
			
			std::shared_ptr<CraftBoxTokenFetcher> crafter_box_token_fetcher = std::make_shared<CraftBoxTokenFetcher>();
			auto cookie = BoxUserInfoManager::global()->user_cookie();

			std::shared_ptr<LoadingWnd> win = std::make_shared<LoadingWnd>();
			std::weak_ptr<LoadingWnd> weak_obj = win;
			std::function<void()> cb = [weak_obj]()
			{
				auto obj = weak_obj.lock();
				if (!!obj)
					obj->Close(100);
			};

			crafter_box_token_fetcher->fetch_finished.connect(win.get(), cb);
			win->Create(this->GetHWND(), NULL, WS_VISIBLE | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_TOOLWINDOW);
			win->CenterWindow();
			crafter_box_token_fetcher->begin_fetch(cookie, pid_str);
			win->ShowModal();

			auto token_info = crafter_box_token_fetcher->player_token_info();
			if (!!token_info)
			{
				if (!token_info->err_msg.empty())
				{
					::MessageBoxW(GetHWND(), token_info->err_msg.c_str(), L"错误", MB_OK | MB_ICONERROR);
					return;
				}

				user_identity = MinecraftUserIdentityFactory::create_user_identity_object_with_player_name(login_type, token_info->player_name);

				cmd = "/dl ";
				USES_CONVERSION;
				cmd += ATL::CW2A(token_info->session.c_str());
				std::wstring tips;
				if (token_info->ban == "2")
				{
					tips = L"若您是该服的老玩家，需要先用旧密码登陆1次~\n您的登陆码为【" + token_info->session + L"】\n已复制登陆指令到粘贴板，进服后粘贴即可！\n部分客户端已经实现自动登陆~ 感谢使用！";
				}
				else
				{
					tips = L"您的登陆码为【" + token_info->session + L"】\n已复制登陆指令到粘贴板，进服后粘贴即可！\n部分客户端已经实现自动登陆~感谢使用！";
				}
				
				SystemHelper().WriteClipBoard(this->GetHWND(), cmd);
				::MessageBoxW(GetHWND(), tips.c_str(), L"重要提示", MB_OK|MB_ICONINFORMATION);
			}
			else
			{
				::MessageBoxW(GetHWND(), L"无法获取验证码, 请稍后重试", L"错误", MB_OK | MB_ICONERROR);
				return;
			}

			break;
		}

		case mcfe::kOfficial:
		{
			auto yggdrasil_auth = YggdrasilAuthentication::global();
			if (yggdrasil_auth->authenticate_state() != YggdrasilAuthentication::kValidated)
			{
				::MessageBoxW(this->GetHWND(), L"请先进行正版登录认证", L"提示", MB_OK | MB_ICONINFORMATION);
				show_config_wnd(ConfigWindow::kGameSetting, true);
				return;
			}
			user_identity = MinecraftUserIdentityFactory::create_user_identity_object(login_type);
			break;
		}
		default:
			return;
		}

		std::wstring ip_and_port;
		std::vector<std::pair<std::wstring, std::wstring>> extern_params;
		if (!server.empty())
		{
			ip_and_port += server;
			extern_params.push_back(std::make_pair(L"server", server));
		}

		if (port > 0 && port < 65536)
		{
			std::wstring port_str = std::to_wstring(port);
			ip_and_port += (L":"+port_str);
			extern_params.push_back(std::make_pair(L"port", port_str));
		}

		if (!ip_and_port.empty() && !server_name.empty())
		{
			add_server_list_history(java_path, profile->client_dir() + L"\\servers.dat", ip_and_port + L"|" + server_name);
		}
		else if (DefaultServerList::instance()->is_fetch_finished())
		{
			DefaultServerList *inst = DefaultServerList::instance();
			auto server_list = inst->server_list();
			std::string params;
			for (auto it = server_list.begin(); it != server_list.end(); it++)
			{
				params += it->host + "|" + it->name + ",";
			}

			std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
			std::wstring params_unicode = converter.from_bytes(params.substr(0, params.length()-1));
			add_server_list_history(java_path, profile->client_dir() + L"\\servers.dat", params_unicode);
		}
		
		std::weak_ptr<MainWnd> weak_obj = this->shared_from_this();
		auto launcher_process_proc = [weak_obj, user_identity, login_type, profile, java_path , server_name, extern_params, cmd]()
		{
			auto obj = weak_obj.lock();
			if (!!obj)
			{
				MinecraftInstanceManager::GetInstance()->set_default_instance(profile->base_dir(), profile->version_str());

				std::shared_ptr<LoadingWnd> win = std::make_shared<LoadingWnd>();

				std::wstring java_param = L"-XX:+UseG1GC -XX:-UseAdaptiveSizePolicy -XX:HeapDumpPath=MojangTricksIntelDriversForPerformance_javaw.exe_minecraft.exe.heapdump -XX:-OmitStackTraceInFastThrow -Xmn128m";
				
				auto java_xmx = JavaEnvironment::GetInstance()->java_Xmx_MB();
				java_param += (L" -Xmx" + std::to_wstring(java_xmx) + L"m");

				std::shared_ptr<MinecraftLauncher> minecraft_launcher = std::make_shared<MinecraftLauncher>(profile, user_identity, java_path, java_param, extern_params);

				std::weak_ptr<LoadingWnd> weak_obj = win;
				std::function<void()> cb = [weak_obj]()
				{
					auto obj = weak_obj.lock();
					if (!!obj)
						obj->Close(100);
				};
				minecraft_launcher->launch_processed.connect(win.get(), cb);
				win->Create(obj->GetHWND(), NULL, WS_VISIBLE | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_TOOLWINDOW);
				win->CenterWindow();
				if (minecraft_launcher->launch())
					win->ShowModal();

				if (!minecraft_launcher->launch_success())
				{
					switch (minecraft_launcher->error())
					{
					case MinecraftLauncher::kErrorFileLoss:
						::MessageBox(obj->GetHWND(), L"部分游戏文件丢失, 你可以尝试使用游戏修复功能", L"启动失败", MB_ICONINFORMATION | MB_OK);
						break;
					case MinecraftLauncher::kErrorCannotCreateProcess:
						::MessageBox(obj->GetHWND(), L"无法创建游戏进程", L"启动失败", MB_ICONERROR | MB_OK);
						break;
					}
				}
				else
				{
					auto process_info = minecraft_launcher->proc_info();

					{
						std::thread t([process_info, server_name, cmd, login_type]()
						{
							int i = 0;
							do 
							{
								WindowFinder finder;
								finder.set_process_id(process_info->proc_id_);
								finder.set_window_class_name(L"LWJGL");
								HWND wnd = finder.find();
								if (wnd != (HWND)-1)
								{
									process_info->main_wnd_ = wnd;
									std::wstring s_name = server_name;
									if (s_name.empty())
									{
										wchar_t buf[1025] = { 0 };
										GetWindowTextW(wnd, buf, 1024);
										s_name = buf;
									}
									else
										::SendMessage(process_info->main_wnd_, WM_SETTEXT, 0, (LPARAM)s_name.c_str());

									if (login_type == mcfe::kCraftBox && !cmd.empty())
									{
										std::wstring login_tips(cmd.begin(), cmd.end());
										login_tips = s_name + L" 【登陆指令: " + login_tips + L"】";
										for (int j = 0; j < 15; j++)
										{
											if((j & 1) == 0)
												::SendMessage(process_info->main_wnd_, WM_SETTEXT, 0, (LPARAM)s_name.c_str());
											else
												::SendMessage(process_info->main_wnd_, WM_SETTEXT, 0, (LPARAM)login_tips.c_str());

											Sleep(800);
										}
									}

									break;
								}

								Sleep(3000);
								
							} while (++i<=2);
							
						});
						t.detach();
					}

					if (login_type == mcfe::kCraftBox)
					{
						std::thread t([process_info, cmd]()
						{
							int i = 1;
							while (i <= 10)
							{
								HWND ipc_msg_wnd = ::FindWindowExW(HWND_MESSAGE, 0, L"craftbox_ipcwin", std::to_wstring(process_info->proc_id_).c_str());
								if (ipc_msg_wnd != 0)
								{
									process_info->ipc_trans_.reset(new IPCTransform(ipc_msg_wnd));
									process_info->ipc_trans_->send_ipc_message(IPC_MSG_LOGIN, cmd.c_str(), cmd.length());
									break;
								}

								Sleep(500 * i);
								i++;
							}

						}
						);
						t.detach();
					}

					if (!!process_info->stderr_cb_)
						process_info->stderr_cb_->start();
					if (!!process_info->stdout_cb_)
						process_info->stdout_cb_->start();

					MinecraftProcessInfoManager::global()->add_process_info(process_info);

					::ShowWindow(obj->GetHWND(), SW_SHOWMINIMIZED);
				}

				minecraft_launcher->wait_thread();
			}

		};

		AsyncTaskDispatch::current()->post_task(make_closure(launcher_process_proc), true);
	}
	else
	{
		::MessageBoxW(this->GetHWND(), L"无法正确解析游戏启动参数文件", L"启动失败", MB_ICONERROR | MB_OK);
	}
}

void MainWnd::on_user_header_pic_fetch_state_changed(std::shared_ptr<NetworkTask> task, NetworkTask::State state)
{
	if (state == NetworkTask::kStop)
	{
		auto dl = std::dynamic_pointer_cast<HttpDownloadToFileWithCache>(task);
		if (dl->download_success() && BoxUserInfoManager::global()->is_logined())
		{
			wstring image_path = dl->download_file_path();
			std::weak_ptr<MainWnd> main_wnd = this->shared_from_this();
			AsyncTaskDispatch::main_thread()->post_delay_task(make_closure([main_wnd, image_path]()
			{
				auto obj = main_wnd.lock();
				if (!!obj && obj->IsWindow())
				{
					obj->user_header_btn_->SetBkImage(image_path.c_str());
				}
				
			}), false, 500);
			
		}
	}
}

void MainWnd::refresh_official_game_list(bool only_show_release)
{
	official_game_list_->RemoveAll();
	
	if (!official_mc_vers_->update_success())
		return;

	auto versions = official_mc_vers_->version_list();
	for (auto it = versions.begin(); it != versions.end(); ++it)
	{
		if (only_show_release && (*it)->type != OfficialMinecraftVersions::kRelease)
			continue;

		CListContainerElementUI* pListElement = nullptr;
		if (!official_game_list_Builder_.GetMarkup()->IsValid())
		{
			pListElement = static_cast<CListContainerElementUI*>(official_game_list_Builder_.Create(_T("official_game_list_item.xml"), (UINT)0, nullptr, &m_PaintManager));
		}
		else
		{
			pListElement = static_cast<CListContainerElementUI*>(official_game_list_Builder_.Create((UINT)0, &m_PaintManager));
		}
		DCHECK(pListElement);

		CLabelUI* version = static_cast<CButtonUI*>(m_PaintManager.FindSubControlByName(pListElement, L"version"));
		if (version)
		{
			USES_CONVERSION;
			version->SetText(ATL::CA2W((*it)->id.c_str()));
		}

		CLabelUI* type = static_cast<CButtonUI*>(m_PaintManager.FindSubControlByName(pListElement, L"type"));
		if (type)
		{
			const wchar_t *type_str = L"";
			switch ((*it)->type)
			{
			case OfficialMinecraftVersions::kRelease:
				type_str = L"正式版";
				break;
			case OfficialMinecraftVersions::kSnapshot:
				type_str = L"快照版";
				break;
			case OfficialMinecraftVersions::kOldAlpha:
			case OfficialMinecraftVersions::kOldBeta:
				type_str = L"远古版";
				break;
			default:
				break;
			}
			type->SetText(type_str);
		}

		CLabelUI* release_time = static_cast<CButtonUI*>(m_PaintManager.FindSubControlByName(pListElement, L"release_time"));
		if (version)
		{
			USES_CONVERSION;
			release_time->SetText(ATL::CA2W((*it)->release_time.c_str()));
		}

		CButtonUI* dl_btn = static_cast<CButtonUI*>(m_PaintManager.FindSubControlByName(pListElement, L"download"));
		if (dl_btn)
		{
			dl_btn->AddCustomAttribute(L"download_type", L"0");
			USES_CONVERSION;
			dl_btn->AddCustomAttribute(L"url", ATL::CA2W((*it)->url.c_str()));
			dl_btn->AddCustomAttribute(L"version", ATL::CA2W((*it)->id.c_str()));
		}

		official_game_list_->Add(pListElement);
	}
}

void MainWnd::on_local_game_record_load_finished()
{
	if (!this->IsWindow())
		return;

	auto default_inst = MinecraftInstanceManager::GetInstance()->default_local_inst();
	if (!!default_inst)
	{
		select_game_btn_->SetText(get_ui_text_for_mc_inst(default_inst, false).c_str());
		select_game_btn_->AddCustomAttribute(L"base_dir", default_inst->base_dir.c_str());
		select_game_btn_->AddCustomAttribute(L"version_str", default_inst->version_str.c_str());
		std::wstring text = L"路径: " + default_inst->base_dir + L"\\versions\\" + default_inst->version_str;
		select_game_btn_->SetToolTip(text.c_str());
	}

	if (!mc_inst_search_)
		init_mc_instance_search_obj();

	mc_inst_search_->start_search(true);
}

void MainWnd::launch_server(std::wstring sid, std::wstring server, int port, std::wstring server_name, std::wstring player_name, int account_type, std::string cmd, bool mod_server, std::vector<std::wstring> client_vers)
{
	auto ver_str = select_game_btn_->GetCustomAttribute(L"version_str");
	auto base_dir = select_game_btn_->GetCustomAttribute(L"base_dir");
	if (ver_str == nullptr || base_dir == nullptr)
	{
		MessageBoxW(this->GetHWND(), L"你需要先添加本地游戏,或者直接使用启动器的下载游戏功能", L"找不到游戏哇 ╮(╯▽╰)╭", MB_OK | MB_ICONINFORMATION);
		game_list_->SelectItem(1);
		ui_select_intergrate_game_tab();
		client_page_opt_->Selected(true);
		return;
	}

	std::shared_ptr<MinecraftProfile> profile = std::make_shared<MinecraftProfile>(base_dir, ver_str);
	if (!::PathFileExistsW(profile->client_dir().c_str()) || !::PathIsDirectoryW(profile->client_dir().c_str()))
	{
		::MessageBoxW(this->GetHWND(), L"游戏路径不存在", L"启动失败", MB_ICONERROR | MB_OK);
		return;
	}

	std::wstring java_path = JavaEnvironment::GetInstance()->selected_java_path();
	if (java_path.empty())
	{
		auto javas = JavaEnvironment::GetInstance()->java_paths_and_vers();
		auto it = javas.begin();
		if (it == javas.end())
		{
			::MessageBoxW(this->GetHWND(), L"请先配置Java路径", L"找不到Java", MB_OK | MB_ICONINFORMATION);
			show_config_wnd(ConfigWindow::kGameSetting, true);
			return;
		}
		else
		{
			java_path = it->first;
		}
	}

	auto login_type = mcfe::UserAccountType(account_type);

	if (profile->load_game_profile_file(true))
	{
		if (mod_server)
		{
			if (profile->sid() != sid)
			{
				if (IDNO == ::MessageBoxW(this->GetHWND(), L"你当前所选择的游戏版本好像不是该模组服的整合版本。 仍然要继续吗？ \r\n要继续使用当前版本进行游戏, 请点击\"是\";\r\n要重新选择版本, 请点击\"否\"。", L"提示", MB_ICONINFORMATION | MB_YESNO))
					return;
			}
		}
		else if(!client_vers.empty())
		{
			std::wstring client_jar_path = profile->client_file().file_path;
			if (!PathFileExistsW(client_jar_path.c_str()))
			{
				client_jar_path = profile->client_dir() + L"\\" + profile->version_str() + L".jar";
				if (!::PathFileExistsW(client_jar_path.c_str()))
					client_jar_path.clear();
			}

			bool found_client = false;
			if (!client_jar_path.empty())
			{
				MinecraftVersionDetecter mc_ver_detecter;
				std::string mc_ver;
				if (mc_ver_detecter.get_mc_version(java_path, client_jar_path, mc_ver))
				{
					std::wstring version(mc_ver.begin(), mc_ver.end());
					for (auto it = client_vers.begin(); it != client_vers.end(); it++)
					{
						auto ret = mcfe::version_compare(version, *it);
						if (ret == mcfe::kVersionEqual || ret == mcfe::kLittleVersionMore)
						{
							found_client = true;
							break;
						}
					}
				}
			}

			if (!found_client)
			{
				if (IDNO == ::MessageBoxW(this->GetHWND(), L"你当前所选择的游戏版本跟服务器要求的版本不一致。 仍然要继续吗？ \r\n要继续使用当前版本进行游戏, 请点击\"是\";\r\n要重新选择版本, 请点击\"否\"。", L"提示", MB_ICONINFORMATION | MB_YESNO))
					return;
			}
		}

		std::wstring ip_and_port;
		std::vector<std::pair<std::wstring, std::wstring>> extern_params;
		if (!server.empty())
		{
			ip_and_port += server;
			extern_params.push_back(std::make_pair(L"server", server));
		}
			

		if (port > 0 && port < 65536)
		{
			std::wstring port_str = std::to_wstring(port);
			ip_and_port += (L":" + port_str);
			extern_params.push_back(std::make_pair(L"port", port_str));
		}

		if (!ip_and_port.empty() && !server_name.empty())
		{
			add_server_list_history(java_path, profile->client_dir() + L"\\servers.dat", ip_and_port + L"|" + server_name);
		}
		else if (DefaultServerList::instance()->is_fetch_finished())
		{
			DefaultServerList *inst = DefaultServerList::instance();
			auto server_list = inst->server_list();
			std::string params;
			for (auto it = server_list.begin(); it != server_list.end(); it++)
			{
				params += it->host + "|" + it->name + ",";
			}

			std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
			std::wstring params_unicode = converter.from_bytes(params.substr(0, params.length() - 1));
			add_server_list_history(java_path, profile->client_dir() + L"\\servers.dat", params_unicode);
		}
		
		std::weak_ptr<MainWnd> weak_obj = this->shared_from_this();
		auto launcher_process_proc = [weak_obj, login_type, player_name, profile, java_path, extern_params, server_name, cmd]()
		{
			auto obj = weak_obj.lock();
			if (!!obj)
			{
				std::wstring java_param = L"-XX:+UseG1GC -XX:-UseAdaptiveSizePolicy -XX:HeapDumpPath=MojangTricksIntelDriversForPerformance_javaw.exe_minecraft.exe.heapdump -XX:-OmitStackTraceInFastThrow -Xmn128m";
				if (SystemInfo().is_x64_system())
				{
					auto java_xmx = JavaEnvironment::GetInstance()->java_Xmx_MB();
					java_param += (L" -Xmx" + std::to_wstring(java_xmx) + L"m");
				}
				
				std::shared_ptr<MinecraftUserIdentity> user_identity;
				if (player_name.empty())
					user_identity = MinecraftUserIdentityFactory::create_user_identity_object(login_type);
				else
					user_identity = MinecraftUserIdentityFactory::create_user_identity_object_with_player_name(login_type, player_name);

				DCHECK(!!user_identity);

				std::shared_ptr<MinecraftLauncher> minecraft_launcher = std::make_shared<MinecraftLauncher>(profile, user_identity, java_path, java_param, extern_params);

				std::shared_ptr<LoadingWnd> win = std::make_shared<LoadingWnd>();
				std::weak_ptr<LoadingWnd> weak_obj = win;
				std::function<void()> cb = [weak_obj]()
				{
					auto obj = weak_obj.lock();
					if (!!obj)
						obj->Close(100);
				};
				minecraft_launcher->launch_processed.connect(win.get(), cb);
				win->Create(obj->GetHWND(), NULL, WS_VISIBLE | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_TOOLWINDOW);
				win->CenterWindow();
				if (minecraft_launcher->launch())
					win->ShowModal();

				if (!minecraft_launcher->launch_success())
				{
					switch (minecraft_launcher->error())
					{
					case MinecraftLauncher::kErrorFileLoss:
						::MessageBox(obj->GetHWND(), L"部分游戏文件丢失, 你可以尝试使用游戏修复功能", L"启动失败", MB_ICONINFORMATION | MB_OK);
						break;
					case MinecraftLauncher::kErrorCannotCreateProcess:
						::MessageBox(obj->GetHWND(), L"无法创建游戏进程", L"启动失败", MB_ICONERROR | MB_OK);
						break;
					}
				}
				else
				{
					auto process_info = minecraft_launcher->proc_info();

					{
						std::thread t([process_info, server_name, login_type, cmd]()
						{
							int i = 0;
							do
							{
								WindowFinder finder;
								finder.set_process_id(process_info->proc_id_);
								finder.set_window_class_name(L"LWJGL");
								HWND wnd = finder.find();
								if (wnd != (HWND)-1)
								{
									process_info->main_wnd_ = wnd;
									std::wstring s_name = server_name;
									if (s_name.empty())
									{
										wchar_t buf[1025] = { 0 };
										GetWindowTextW(wnd, buf, 1024);
										s_name = buf;
									}
									else
										::SendMessage(process_info->main_wnd_, WM_SETTEXT, 0, (LPARAM)s_name.c_str());

									if (login_type == mcfe::kCraftBox && !cmd.empty())
									{
										std::wstring login_tips(cmd.begin(), cmd.end());
										login_tips = s_name + L" 【登陆指令: " + login_tips + L"】";
										for (int j = 0; j < 15; j++)
										{
											if ((j & 1) == 0)
												::SendMessage(process_info->main_wnd_, WM_SETTEXT, 0, (LPARAM)s_name.c_str());
											else
												::SendMessage(process_info->main_wnd_, WM_SETTEXT, 0, (LPARAM)login_tips.c_str());

											Sleep(800);
										}
									}

									break;
								}

								Sleep(3000);

							} while (++i <= 2);

						});
						t.detach();
					}

					if (login_type == mcfe::kCraftBox)
					{
						std::thread t([process_info, cmd]()
						{
							int i = 1;
							while (i <= 10)
							{
								HWND ipc_msg_wnd = ::FindWindowExW(HWND_MESSAGE, 0, L"craftbox_ipcwin", std::to_wstring(process_info->proc_id_).c_str());
								if (ipc_msg_wnd != 0)
								{
									process_info->ipc_trans_.reset(new IPCTransform(ipc_msg_wnd));
									process_info->ipc_trans_->send_ipc_message(IPC_MSG_LOGIN, cmd.c_str(), cmd.length());
									break;
								}
								
								Sleep(500 * i);
								i++;
							}
							
						}
						);
						t.detach();
					}
													
					if (!!process_info->stderr_cb_)
						process_info->stderr_cb_->start();
					if (!!process_info->stdout_cb_)
						process_info->stdout_cb_->start();

					MinecraftProcessInfoManager::global()->add_process_info(process_info);

					::ShowWindow(obj->GetHWND(), SW_SHOWMINIMIZED);
				}

				minecraft_launcher->wait_thread();
			}
		};

		auto app_config = AppConfig::global();
		switch (login_type)
		{
		case mcfe::kOffline:
		{
			if (player_name.empty())
			{
				std::wstring offline_user_name = app_config->get_value_for_key(L"offline_player_name");
				if (offline_user_name.empty()/* || !verify_offline_user_name(offline_user_name)*/)
				{
					::MessageBoxW(this->GetHWND(), L"离线模式的角色名字不能为空", L"请重新设置离线模式的角色名字", MB_OK | MB_ICONINFORMATION);
					show_config_wnd(ConfigWindow::kGameSetting, true);
					return;
				}
			}

			AsyncTaskDispatch::current()->post_task(make_closure(launcher_process_proc), true);

			break;
		}

		case mcfe::kOfficial:
		{
			if (YggdrasilAuthentication::global()->authenticate_state() == YggdrasilAuthentication::kValidated)
			{
				AsyncTaskDispatch::current()->post_task(make_closure(launcher_process_proc), true);
			}
			else
			{
				::MessageBoxW(GetHWND(), L"请先进行正版登录认证", L"提示", MB_OK | MB_ICONINFORMATION);
				show_config_wnd(ConfigWindow::kGameSetting, true);
				return;
			}

			break;
		}

		default:

			AsyncTaskDispatch::current()->post_task(make_closure(launcher_process_proc), true);
			break;
		}
	}
	else
	{
		::MessageBoxW(this->GetHWND(), L"无法正确解析游戏启动参数文件", L"启动失败", MB_ICONERROR | MB_OK);
	}
}

void MainWnd::save_cookie(const std::string &cookie)
{
	UserAcountPersistence persistence;
	persistence.save(cookie);
}

void MainWnd::load_cookie(std::string &cookie)
{
	UserAcountPersistence persistence;
	persistence.load(cookie);
}

void MainWnd::on_fetching_user_info()
{
	login_btn_->SetText(L"登陆中...");
	login_btn_->SetEnabled(false);
	user_header_btn_->SetEnabled(false);
}

void MainWnd::on_fetch_user_info_finished(bool success)
{
	if (success)
	{
		auto user_info = BoxUserInfoManager::global()->user_info();
		std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
		login_btn_->SetText(converter.from_bytes(user_info->user_name).c_str());
		login_btn_->SetEnabled(true);
		user_header_btn_->SetEnabled(true);
		 
		auto &header_url = user_info->head_url;
		if (!header_url.empty())
		{
			on_modify_header(header_url);
		}

		CraftBoxUserJoinServerInfos::instance()->update_craftbox_user_join_server_infos(L"", L"");
	}
	else
	{
		login_btn_->SetText(L"登录 / 注册");
		login_btn_->SetEnabled(true);
		user_header_btn_->SetEnabled(true);
	}
}

void MainWnd::on_logout()
{
	login_btn_->SetText(L"登录 / 注册");
	login_btn_->SetEnabled(true);
	user_header_btn_->SetBkImage(default_header_pic_.c_str());
	user_header_btn_->SetEnabled(true);

	CraftBoxUserJoinServerInfos::instance()->clear();
}

void MainWnd::on_download_item_state_changed(std::shared_ptr<DownloadingItemController> item, DownloadingItemController::State state)
{
	auto it = dl_info_map_.find(item.get());
	if (it == dl_info_map_.end())
	{
		DCHECK(0);
		return;
	}

	auto &info = it->second;

	switch (item->state())
	{
	case DownloadingItemController::kFailed:
	{
		info->retry_btn->SetVisible(true);
		info->start_btn->SetVisible(false);
		info->pause_btn->SetVisible(false);
		info->cancel_btn->SetVisible(true);
		info->progress_label->SetText(L"下载失败");

		std::wstring msg = L"\"" + info->version + L"\"下载过程发生错误, 请确保网络正常, 下载目录的文件没有被占用, 目录所在分区剩余空间足够并且目录具有写权限。你可以稍后再重试下载";
		notify_icon_->show_notify_message(msg.c_str(), mcfe::g_app_name, 10 * 1000);
		std::thread play_sound_thread([]()
		{
			std::wstring command = PathService().appdata_path() + L"\\dl_failed.mp3";
			command = L"open " + command + L" alias pl_failed";
			mciSendStringW(command.c_str(), nullptr, 0, nullptr);
			mciSendStringW(L"play pl_failed wait", nullptr, 0, nullptr);
		});
		play_sound_thread.detach();
	}
	break;
	case DownloadingItemController::kUnZipping:
		info->retry_btn->SetVisible(false);
		info->start_btn->SetVisible(false);
		info->pause_btn->SetVisible(true);
		info->cancel_btn->SetVisible(true);
		info->progress_label->SetText(L"正在解压...");
		break;
	case DownloadingItemController::kPaused:
		info->retry_btn->SetVisible(false);
		info->start_btn->SetVisible(true);
		info->pause_btn->SetVisible(false);
		info->cancel_btn->SetVisible(true);
		info->progress_label->SetText(L"已暂停");
		break;
	case DownloadingItemController::kDownloading:
		info->retry_btn->SetVisible(false);
		info->start_btn->SetVisible(false);
		info->pause_btn->SetVisible(true);
		info->cancel_btn->SetVisible(true);
		if (info->progress_label->GetText().IsEmpty())
			info->progress_label->SetText(L"计算中...");
		break;
	case DownloadingItemController::kWaiting:
		info->retry_btn->SetVisible(false);
		info->start_btn->SetVisible(false);
		info->pause_btn->SetVisible(true);
		info->cancel_btn->SetVisible(true);
		info->progress_label->SetText(L"排队中...");
		break;
	case DownloadingItemController::kFinish:
	{
		downloading_list_->Remove(info->list_item, true);
		DownloadingItemController *dl_item = info->item.get();
		std::weak_ptr<MainWnd> weak_obj = this->shared_from_this();
		AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj, dl_item]()
		{
			auto obj = weak_obj.lock();
			if (!!obj)
			{
				auto it = obj->dl_info_map_.find(dl_item);
				if (it != obj->dl_info_map_.end())
				{
					if (it->second->item->type() == DownloadingItemController::kOfficial)
					{
						auto item = std::dynamic_pointer_cast<OfficialMinecraftDownloadItemController>(it->second->item);
						auto inst = item->download_obj();
						MinecraftInstanceManager::GetInstance()->remove_download_instance(inst);
						MinecraftInstanceManager::GetInstance()->check_and_add_minecraft_instance(inst->base_dir(), inst->version_str());
					}
					else if (it->second->item->type() == DownloadingItemController::kThirdParty)
					{
						auto item = std::dynamic_pointer_cast<IntergrationDownloadItemController>(it->second->item);
						auto inst = item->download_obj();
						IntergrationVersionManager::global()->remove_download_instance(inst);
						MinecraftInstanceManager::GetInstance()->check_and_add_minecraft_instance(item->file_path()+L"\\.minecraft");
					}

					it->second->item.reset();
					obj->dl_info_map_.erase(it);
				}
				else
				{
					DCHECK(0);
				}
			}

		}), false);

		std::wstring msg = L"\"" + info->version + L"\"下载成功";
		notify_icon_->show_notify_message(msg.c_str(), mcfe::g_app_name);
		std::thread play_sound_thread([]()
		{
			std::wstring command = PathService().appdata_path() + L"\\dl_success.mp3";
			command = L"open " + command + L" alias pl_suc";
			mciSendStringW(command.c_str(), nullptr, 0, nullptr);
			mciSendStringW(L"play pl_suc wait", nullptr, 0, nullptr);
		});
		play_sound_thread.detach();

	}

	break;

	case DownloadingItemController::kNeedRemove:
	{
		downloading_list_->Remove(info->list_item, true);
		DownloadingItemController *dl_item = info->item.get();
		std::weak_ptr<MainWnd> weak_obj = this->shared_from_this();
		AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj, dl_item]()
		{
			auto obj = weak_obj.lock();
			if (!!obj)
			{
				auto it = obj->dl_info_map_.find(dl_item);
				if (it != obj->dl_info_map_.end())
				{
					if (it->second->item->type() == DownloadingItemController::kOfficial)
					{
						auto item = std::dynamic_pointer_cast<OfficialMinecraftDownloadItemController>(it->second->item);
						auto inst = item->download_obj();
						MinecraftInstanceManager::GetInstance()->remove_download_instance(inst);
					}
					else if (it->second->item->type() == DownloadingItemController::kThirdParty)
					{
						auto item = std::dynamic_pointer_cast<IntergrationDownloadItemController>(it->second->item);
						auto inst = item->download_obj();
						IntergrationVersionManager::global()->remove_download_instance(inst);
					}

					it->second->item.reset();
					obj->dl_info_map_.erase(it);
				}
				else
				{
					DCHECK(0);
				}
			}

		}), false);

	}
	break;
	}
}

void MainWnd::on_intergration_downloading_instance_added(std::shared_ptr<HttpResumableDownloadToFile> dl_inst, std::string file_sha1, uint64_t file_size, std::wstring file_save_path, std::wstring verstion_str)
{
	CListContainerElementUI* pListElement = nullptr;
	if (!donwloading_list_builder_.GetMarkup()->IsValid())
	{
		pListElement = static_cast<CListContainerElementUI*>(donwloading_list_builder_.Create(_T("downloading_list_item.xml"), (UINT)0, nullptr, &m_PaintManager));
	}
	else
	{
		pListElement = static_cast<CListContainerElementUI*>(donwloading_list_builder_.Create((UINT)0, &m_PaintManager));
	}
	DCHECK(pListElement);

	std::shared_ptr<IntergrationDownloadItemController> dl_item_ctrl = std::make_shared<IntergrationDownloadItemController>(dl_inst);
	dl_item_ctrl->state_changed.connect(this, &MainWnd::on_download_item_state_changed);
	dl_item_ctrl->set_file_path(file_save_path);
	dl_item_ctrl->set_file_size(file_size);
	dl_item_ctrl->set_file_sha1(file_sha1);

	std::wstring path_text = L"保存路径: " + file_save_path;
	CLabelUI* name = static_cast<CLabelUI*>(m_PaintManager.FindSubControlByName(pListElement, L"name"));
	if (name)
	{
		name->SetText(verstion_str.c_str());
		name->SetToolTip(path_text.c_str());
	}

	std::shared_ptr<DownloadItemAndAssociateUI> info(new DownloadItemAndAssociateUI);
	info->item = dl_item_ctrl;
	info->version = verstion_str;
	CLabelUI* progress = static_cast<CLabelUI*>(m_PaintManager.FindSubControlByName(pListElement, L"progress"));
	if (progress)
	{
		info->progress_label = progress;
	}
	CButtonUI* cancel_button = static_cast<CButtonUI*>(m_PaintManager.FindSubControlByName(pListElement, L"cancel_download"));
	if (cancel_button)
	{
		cancel_button->SetTag((UINT_PTR)(DownloadingItemController*)dl_item_ctrl.get());
		info->cancel_btn = cancel_button;
	}
	CButtonUI* start_dl_button = static_cast<CButtonUI*>(m_PaintManager.FindSubControlByName(pListElement, L"start_download"));
	if (start_dl_button)
	{
		start_dl_button->SetTag((UINT_PTR)(DownloadingItemController*)dl_item_ctrl.get());
		info->start_btn = start_dl_button;
	}
	CButtonUI* pause_button = static_cast<CButtonUI*>(m_PaintManager.FindSubControlByName(pListElement, L"pause_download"));
	if (pause_button)
	{
		pause_button->SetTag((UINT_PTR)(DownloadingItemController*)dl_item_ctrl.get());
		info->pause_btn = pause_button;
	}
	CButtonUI* retry_button = static_cast<CButtonUI*>(m_PaintManager.FindSubControlByName(pListElement, L"retry_download"));
	if (retry_button)
	{
		retry_button->SetTag((UINT_PTR)(DownloadingItemController*)dl_item_ctrl.get());
		info->retry_btn = retry_button;
	}

	info->list_item = pListElement;

	dl_info_map_[(DownloadingItemController*)dl_item_ctrl.get()] = info;

	switch (info->item->state())
	{
	case DownloadingItemController::kPaused:
		info->retry_btn->SetVisible(false);
		info->start_btn->SetVisible(true);
		info->pause_btn->SetVisible(false);
		info->cancel_btn->SetVisible(true);
		info->progress_label->SetText(L"已暂停");
		break;
	case DownloadingItemController::kDownloading:
		info->retry_btn->SetVisible(false);
		info->start_btn->SetVisible(false);
		info->pause_btn->SetVisible(true);
		info->cancel_btn->SetVisible(true);
		if (info->progress_label->GetText().IsEmpty())
			info->progress_label->SetText(L"计算中...");
		break;
	case DownloadingItemController::kWaiting:
		info->retry_btn->SetVisible(false);
		info->start_btn->SetVisible(false);
		info->pause_btn->SetVisible(true);
		info->cancel_btn->SetVisible(true);
		info->progress_label->SetText(L"排队中...");
		break;
	default:
		DCHECK(0);
	}

	downloading_list_->Add(pListElement);

	if (downloading_list_refresh_timer_ == 0)
	{
		downloading_list_refresh_timer_ = ::SetTimer(this->GetHWND(), REFRESH_DOWNLOADING_TIMER, 1000, nullptr);
	}
}

void MainWnd::on_modify_header(const std::string &header_url)
{
	std::shared_ptr<HttpDownloadToFileWithCache> dl = std::make_shared<HttpDownloadToFileWithCache>();
	dl->set_url(header_url);
	std::wstring guid;
	guid_wstring(guid);
	dl->set_download_file_path(PathService().appdata_path() + L"\\temp\\" + guid);
	dl->state_changed.connect(this, &MainWnd::on_user_header_pic_fetch_state_changed);
	dl->start_download();
}

void MainWnd::on_really_close()
{
	if (downloading_list_refresh_timer_ != 0)
	{
		::KillTimer(this->GetHWND(), downloading_list_refresh_timer_);
		downloading_list_refresh_timer_ = 0;
	}

	notify_icon_->hide_notify_icon();
}

void MainWnd::show_quit_confirm_window()
{
	QuitConfirmWnd quit_confirm_wnd;
	if (0 != quit_confirm_wnd.Create(this->GetHWND(), NULL, WS_VISIBLE | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_TOOLWINDOW))
	{
		quit_confirm_wnd.CenterWindow();
		quit_confirm_wnd.ShowModal();
		switch (quit_confirm_wnd.get_select_action())
		{
		case QuitConfirmWnd::kNotSelect:
			break;
		case QuitConfirmWnd::kClose:
			need_really_quit_ = true;
			AppConfig::global()->set_value_for_key(L"on_close", L"2");
			this->Close();
			break;
		case QuitConfirmWnd::kMinial:
			AppConfig::global()->set_value_for_key(L"on_close", L"1");
			this->ShowWindow(false);
			break;
		default:
			break;
		}

		if (quit_confirm_wnd.is_always_action())
			AppConfig::global()->set_value_for_key(L"always_on_close", L"1");
	}
}

void MainWnd::on_java_record_load_finished()
{
	if (this->IsWindow())
	{
		auto javas = JavaEnvironment::GetInstance()->java_paths_and_vers();
		if (javas.empty())
		{
			auto java_search = JavaEnvironment::GetInstance()->search_java();
			java_search->begin_search();
		}
		else
		{
			JavaEnvironment::GetInstance()->reset_select_java_path();
		}
	}
	
}

void MainWnd::on_load_yggdraisl_account_finished(const std::string &access_token, const std::string &client_token)
{
	if (!this->IsWindow())
		return;

	if (!access_token.empty() && !client_token.empty())
	{
		YggdrasilAuthentication::global()->refresh(access_token, client_token);
	}
}

bool MainWnd::verify_offline_user_name(const std::wstring &player_name)
{
	bool ok = true;
	int len = 0;
	const wchar_t* ptr = player_name.c_str();
	while (*ptr != 0)
	{
		len += 1;
		if (len > 16)
		{
			ok = false;
			break;
		}

		if ((*ptr >= L'0' && *ptr <= L'9')
			|| (*ptr >= L'a' && *ptr <= L'z')
			|| (*ptr >= L'A' && *ptr <= L'Z')
			|| *ptr == L'_'
			)
		{
			ptr++;
			continue;
		}

		ok = false;
		break;
	}

	if (ok && len < 3)
		ok = false;

	return ok;
}

void MainWnd::show_user_center_page()
{
	main_page_opt_->Selected(false);
	client_page_opt_->Selected(false);
	server_page_opt_->Selected(false);
	download_page_opt_->Selected(false);
	main_page_->SelectItem(4);
	wb_navigator_layout_->SetVisible(true);
	on_wb_command_state_changed(user_center_event_handler_.get());

	if (first_show_user_page_)
	{
		first_show_user_page_ = false;
		user_page_wb_->Navigate2(mcfe::g_user_page);
	}

	CComBSTR url;
	auto web_browser = user_page_wb_->GetWebBrowser2();
	if (web_browser && SUCCEEDED(web_browser->get_LocationURL(&url)))
	{
		std::wstring url_str((BSTR)url, url.Length());
		if (0 == url_str.find(L"http://user.zuimc.com/serverlist") || 0 == url_str.find(L"http://user.zuimc.com/sid"))
			user_page_wb_->Navigate2(mcfe::g_user_page);
	}
}

void MainWnd::on_wb_command_state_changed(CraftBoxPageEventHandler *obj)
{
	auto sel = main_page_->GetCurSel();
	if (sel == 0)
	{
		if (obj == main_page_event_handler_.get())
		{
			auto wb = main_page_wb_->GetWebBrowser2();
			backward_btn_->SetEnabled(obj->backward_enable_);
			backward_btn_->SetTag((UINT_PTR)wb);
			forward_btn_->SetEnabled(obj->forward_enable_);
			forward_btn_->SetTag((UINT_PTR)wb);
			refresh_btn_->SetTag((UINT_PTR)wb);
		}
	}
	else if (sel == 2)
	{
		if (obj == server_page_event_handler_.get())
		{
			auto wb = server_page_wb_->GetWebBrowser2();
			backward_btn_->SetEnabled(obj->backward_enable_);
			backward_btn_->SetTag((UINT_PTR)wb);
			forward_btn_->SetEnabled(obj->forward_enable_);
			forward_btn_->SetTag((UINT_PTR)wb);
			refresh_btn_->SetTag((UINT_PTR)wb);
		}
	}
	else if (sel == 4)
	{
		if (obj == user_center_event_handler_.get())
		{
			auto wb = user_page_wb_->GetWebBrowser2();
			backward_btn_->SetEnabled(obj->backward_enable_);
			backward_btn_->SetTag((UINT_PTR)wb);
			forward_btn_->SetEnabled(obj->forward_enable_);
			forward_btn_->SetTag((UINT_PTR)wb);
			refresh_btn_->SetTag((UINT_PTR)wb);
		}
	}
}

void MainWnd::handle_command_line(LPCWSTR cmdline, bool from_startup)
{
	LPWSTR *szArglist = NULL;
	int nArgs = 0;
	szArglist = CommandLineToArgvW(cmdline, &nArgs);
	if (NULL != szArglist && nArgs > 1)
	{
		for (auto index = 0; index < nArgs; index++)
		{
			auto ptr = wcsstr(szArglist[index], L"mc://showsid/?"); // showsid/?ssid=<COOKIE键值对>&sidurl=<服务器URL>
			if (ptr == szArglist[index])
			{
				auto val = szArglist[index] + wcslen(L"mc://showsid/?");
				auto cookie_begin = wcsstr(val, L"ssid=");
				if (cookie_begin != nullptr)
				{
					cookie_begin += wcslen(L"ssid=");
					std::wstring cookie_encoded;
					auto cooike_end = wcsstr(cookie_begin, L"&");
					if (cooike_end == nullptr)
						cookie_encoded.assign(cookie_begin);
					else if(cooike_end > cookie_begin)
						cookie_encoded.assign(cookie_begin, cooike_end - cookie_begin);

					if (!cookie_encoded.empty())
					{
						USES_CONVERSION;
						std::string cookie = UrlDecode(std::string(ATL::CW2A(cookie_encoded.c_str())));
						login_with_cookie(cookie);
					}
				}

				auto server_url_begin = wcsstr(val, L"sidurl=");
				if (server_url_begin != nullptr)
				{
					server_url_begin += wcslen(L"sidurl=");
					auto server_url_end = wcsstr(server_url_begin, L"&");
					std::wstring server_url_encoded;
					if (server_url_end == nullptr)
						server_url_encoded.assign(server_url_begin);
					else if (server_url_end > server_url_begin)
						server_url_encoded.assign(server_url_begin, server_url_end - server_url_begin);

					if (!server_url_encoded.empty())
					{
						USES_CONVERSION;
						std::string server_url = UrlDecode(std::string(ATL::CW2A(server_url_encoded.c_str())));
						std::weak_ptr<MainWnd> weak_obj = this->shared_from_this();
						AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj, server_url]()
						{
							auto obj = weak_obj.lock();
							if (!!obj)
							{
								USES_CONVERSION;
								obj->first_show_server_page_ = false;
								obj->server_page_wb_->Navigate2(ATL::CA2W(server_url.c_str()));
								obj->server_page_opt_->Selected(true);
							}
							
						}), false);
						
					}
				}
			}
		}
	}
	else if(from_startup)
	{
		std::string cookie;
		load_cookie(cookie);
		if (!cookie.empty())
		{
			login_with_cookie(cookie);
		}
	}

	if(szArglist)
		LocalFree(szArglist);
}

void MainWnd::login_with_cookie(const std::wstring &cookie)
{
	size_t begin = 0, end = 0;

	do
	{
		end = cookie.find(L";", begin);
		std::wstring cookie_item;
		if (end == std::string::npos)
		{
			cookie_item =cookie.substr(begin);
		}
		else
		{
			cookie_item = cookie.substr(begin, end - begin);
		}

		auto ret = ::InternetSetCookieW(mcfe::g_user_page, nullptr, cookie_item.c_str());
		if (!ret)
		{
			auto err = ::GetLastError();
			DCHECK(0);
			LOG(ERROR) << "set cookie failed with err:" << err;
		}

		begin = end + 1;
	} while (end != std::wstring::npos);

	std::weak_ptr<MainWnd> weak_obj = this->shared_from_this();
	AsyncTaskDispatch::current()->post_task(make_closure([weak_obj]()
	{
		auto obj = weak_obj.lock();
		if (!!obj && obj->IsWindow())
		{
			CWebBrowserUI* wbs[] = { obj->user_page_wb_, obj->main_page_wb_, obj->server_page_wb_ };
			for (auto & wb : wbs)
			{
				CComVariant var(REFRESH_NORMAL);
				auto wb2 = wb->GetWebBrowser2();
				if (wb2)
					wb2->Refresh2(&var);
			}
		}
		
	}), false);

	{
		auto box_user_info_manager = BoxUserInfoManager::global();
		if (box_user_info_manager->is_logined())
		{
			box_user_info_manager->logout();
		}
		USES_CONVERSION;
		box_user_info_manager->auth_with_cookie(std::string(ATL::CW2A(cookie.c_str())));
	}
}

void MainWnd::login_with_cookie(const std::string &cookie)
{
	USES_CONVERSION;
	
	size_t begin = 0, end = 0;
	
	do
	{
		end = cookie.find(";", begin);
		std::wstring cookie_item;
		if (end == std::string::npos)
		{
			cookie_item = ATL::CA2W(cookie.substr(begin).c_str());
		}
		else
		{
			cookie_item = ATL::CA2W(cookie.substr(begin, end - begin).c_str());
		}
		
		auto ret = ::InternetSetCookieW(mcfe::g_user_page, nullptr, cookie_item.c_str());
		if (!ret)
		{
			auto err = ::GetLastError();
			DCHECK(0);
			LOG(ERROR) << "set cookie failed with err:" << err;
		}

		begin = end + 1;
	} while (end != std::string::npos);

	std::weak_ptr<MainWnd> weak_obj = this->shared_from_this();
	AsyncTaskDispatch::current()->post_task(make_closure([weak_obj]()
	{
		auto obj = weak_obj.lock();
		if (!!obj && obj->IsWindow())
		{
			CWebBrowserUI* wbs[] = { obj->user_page_wb_, obj->main_page_wb_, obj->server_page_wb_ };
			for (auto & wb : wbs)
			{
				CComVariant var(REFRESH_NORMAL);
				auto wb2 = wb->GetWebBrowser2();
				if (wb2)
					wb2->Refresh2(&var);
			}
		}

	}), false);

	{
		auto box_user_info_manager = BoxUserInfoManager::global();
		if (box_user_info_manager->is_logined())
		{
			box_user_info_manager->logout();
		}

		box_user_info_manager->auth_with_cookie(cookie);
	}
}

LRESULT MainWnd::OnCopyData(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	COPYDATASTRUCT *cp = (COPYDATASTRUCT*)(lParam);
	if (cp)
	{
		if (cp->dwData == MSG_PASS_APP_PARAM)
		{
			handle_command_line((LPCWSTR)cp->lpData, false);
		}
	}

	return TRUE;
}

void MainWnd::on_download_server_integrate_pack(std::wstring url, std::wstring server_name, std::string sha1, uint64_t sz)
{
	std::wstring version_str = server_name + L"专用整合包";
	DownloadConfirmWnd dl_confirm_wnd;
	dl_confirm_wnd.set_title(L"下载 " + version_str);
	dl_confirm_wnd.Init(this->GetHWND());
	dl_confirm_wnd.show_tips(false);
	dl_confirm_wnd.CenterWindow();
	dl_confirm_wnd.ShowModal();
	auto dl_path = dl_confirm_wnd.selected_download_path();
	if (!dl_path.empty())
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;

		std::wstring guid_string;
		guid_wstring(guid_string);

		bool exist_before = false;
		auto dl_obj = IntergrationVersionManager::global()->download_instance(converter.to_bytes(url), dl_path + L"\\" + guid_string, sha1, sz, dl_path + L"\\" + version_str, version_str, &exist_before);
		dl_obj->start_download();

		if (!exist_before)
		{
			std::wstring msg = L"下载任务\"" + version_str + L"\"成功添加";
			notify_icon_->show_notify_message(msg.c_str(), mcfe::g_app_name, 3000);

			//on_intergration_downloading_instance_added(dl_obj, version_str, dl_path + L"\\" + version_str, 0, "");
		}
		else
		{
			std::wstring msg = L"下载任务\"" + version_str + L"\"已经存在";
			notify_icon_->show_notify_message(msg.c_str(), mcfe::g_app_name, 3000);
		}
	}
}

void MainWnd::on_new_web_page(std::wstring url, int width, int height, bool lock)
{
	std::shared_ptr<WebPageWnd> web_page = std::make_shared<WebPageWnd>();
	if (0 != web_page->Create(0, nullptr, WS_VISIBLE | WS_DLGFRAME|WS_THICKFRAME | WS_POPUPWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_MAXIMIZEBOX| WS_MINIMIZEBOX, 0))
	{
		web_page->destroyed.connect(this, &MainWnd::on_web_page_destroyed);
		web_page->SetIcon(IDI_ICON_LOGO);
		web_page->CenterWindow();
		::SetWindowPos(*web_page, 0, 0, 0, width,
			height, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
		web_page->CenterWindow();
		web_page->navigate(url.c_str());
		web_page->ShowWindow();

		page_wnds_[web_page.get()] = web_page;
	}
}

void MainWnd::on_web_page_destroyed(WebPageWnd* wnd)
{
	auto it = page_wnds_.find(wnd);
	if (it != page_wnds_.end())
		page_wnds_.erase(it);
	else
	{
		DCHECK(0);
	}
		
}

MainWnd* MainWnd::global()
{
	return global_;
}

void MainWnd::delay_save_all_data()
{
	std::weak_ptr<MainWnd> weak_obj = this->shared_from_this();
	AsyncTaskDispatch::current()->post_delay_task(make_closure([weak_obj]()
	{
		auto obj = weak_obj.lock();
		if (!!obj)
		{
			obj->save_all_data();
			obj->delay_save_all_data();
		}
	}), false, 1800*1000);
}

void MainWnd::add_server_list_history(const std::wstring &java_path, const std::wstring &servers_dat_path, const std::wstring &server_name_and_ips)
{
	static std::wstring mc_server_list_modify_jar_path = MainWnd::MinecraftServerListModifyJarPath();

	std::wstring cmd = L"\"" + java_path + L"\" -cp \"" + mc_server_list_modify_jar_path + L"\" com.zuimc.serverlist.ServerListEdit \"" + servers_dat_path + L"\" \"" + server_name_and_ips + L"\"";

	STARTUPINFO startup_info = { 0 };
	startup_info.cb = sizeof(startup_info);
	startup_info.wShowWindow = SW_HIDE;
	startup_info.dwFlags = STARTF_USESHOWWINDOW;

	PROCESS_INFORMATION pi;

	if (!::CreateProcessW(nullptr, (LPWSTR)cmd.c_str(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startup_info, &pi))
	{
		DWORD err = GetLastError();
		std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
		LOG(ERROR) << " create process with " << converter.to_bytes(cmd) << "failed with err " << err;
		DCHECK(0);
		return ;
	}

	::CloseHandle(pi.hThread);
	::WaitForSingleObject(pi.hProcess, 3000);
	::CloseHandle(pi.hProcess);

}

std::wstring MainWnd::MinecraftServerListModifyJarPath()
{
	std::wstring path = PathService().appdata_path() + L"\\temp";
	std::wstring guid;
	guid_wstring(guid);
	path += L"\\";
	path += guid;
	::SHCreateDirectory(0, path.c_str());
	std::wstring temp_zip_file = path + L"\\sl.7z";
	if (extrace_resource(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDR_FILE_MCSERVERLISTMODIFY_7Z), L"FILE", temp_zip_file))
	{
		ZipCmdProxy zip;
		std::wstring cmd = L"x \"" + temp_zip_file + L"\" -o\"" + path + L"\" -y";
		int ret = zip.run(cmd);
		DCHECK(ret == 0);

		::DeleteFileW(temp_zip_file.c_str());

		std::wstring path_sl = path + L"\\MinecraftServerList.jar";
		std::wstring path_jnbt = path + L"\\JNBT_1.4.jar";
		//锁住文件
		static HANDLE zip_file_lock1 = CreateFileW(path_sl.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
		static HANDLE zip_file_lock2 = CreateFileW(path_jnbt.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);

		return path_sl + L";"+ path_jnbt;
	}
	else
	{
		DCHECK(0);
	}

	return L"";
}

void MainWnd::on_mc_instances_all_revoced()
{
	select_game_btn_->SetText(L"");
	select_game_btn_->SetToolTip(L"");
	select_game_btn_->RemoveCustomAttribute(L"base_dir");
	select_game_btn_->RemoveCustomAttribute(L"version_str");
}

void MainWnd::on_game_crashed(std::shared_ptr<MinecraftProcessInfo> process_info)
{
	GameCrashReportWnd wnd;
	wnd.setGameDir(process_info->base_dir_, process_info->version_dir_);
	if (0 != wnd.Create(0, L"游戏崩溃了...", WS_VISIBLE | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0))
	{
		wnd.CenterWindow();
		wnd.ShowModal();
	}
}

MainWnd* MainWnd::global_ = nullptr;
