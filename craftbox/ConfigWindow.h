#pragma once

#include "Duilib/UIlib.h"
#include "JavaEnvironment.h"
#include "base/sigslot.h"
#include "base/closure.h"
#include "minecraft_common/YggdrasilAuthentication.h"

#include <windows.h>

class ShadowEdge;

class ConfigWindow 
	: public DuiLib::CWindowWnd
	, public DuiLib::INotifyUI
	, public std::enable_shared_from_this<ConfigWindow>
	, public sigslot::has_slots<>
{
public:

	enum ConfigWndTab
	{
		kAppSetting = 0,
		kGameSetting,
	};

	enum LoginTypeTab
	{
		kOffline = 0,
		kOfficial,
		kZuiMC,
	};

	ConfigWindow();
	virtual ~ConfigWindow() = default;

	bool Init(HWND parent_wnd);
	LPCTSTR GetWindowClassName() const { return _T("ConfigWindow"); }
	void ShowWindow(bool bShow = true, bool bTakeFocus = true);
	void ui_select_tab(ConfigWndTab tab)
	{
		switch (tab)
		{
		case kAppSetting:
			app_setting_opt_->Selected(true);
			break;
		case kGameSetting:
			game_setting_opt_->Selected(true);
			break;
		}
	}

protected:

	void on_yggdrasil_auth_state_changed(YggdrasilAuthentication::AuthState state);
	void do_official_login();

	void InitWindow();

	HWND Create(HWND hwndParent, LPCTSTR pstrName, DWORD dwStyle, DWORD dwExStyle, int x = CW_USEDEFAULT, int y = CW_USEDEFAULT, int cx = CW_USEDEFAULT, int cy = CW_USEDEFAULT, HMENU hMenu = NULL);

	virtual LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual LRESULT OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual LRESULT OnMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual LRESULT OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	virtual void Notify(DuiLib::TNotifyUI& msg) override;

	DuiLib::CPaintManagerUI * GetPaintManager() { return &m_pm; }

	void handle_java_list();
	void update_download_progress();

	void on_java_search_finished();
	void on_java_download_success();
	void on_java_download_failed();

	void save_config();

	void on_fetch_user_join_server_info_finished();

private:
	DuiLib::CPaintManagerUI m_pm;
	ShadowEdge* m_pShadowWnd = nullptr;

	DuiLib::COptionUI *app_setting_opt_ = nullptr;
	DuiLib::COptionUI *game_setting_opt_ = nullptr;
	DuiLib::CTabLayoutUI *setting_tab_ = nullptr;

	DuiLib::COptionUI *minial_opt_ = nullptr;
	DuiLib::COptionUI *close_opt_ = nullptr;

	DuiLib::CDialogBuilder default_list_label_builder_;


	DuiLib::CComboUI *game_account_type_cb_ = nullptr;
	DuiLib::CTabLayoutUI *game_account_type_pages_ = nullptr;
	DuiLib::CEditUI * offline_user_name_edit_ = nullptr;
	DuiLib::CEditUI * official_account_edit_ = nullptr;
	DuiLib::CEditUI * official_password_edit_ = nullptr;
	DuiLib::CLabelUI *official_player_name_lb_ = nullptr;
	DuiLib::CButtonUI *official_login_btn_ = nullptr;
	DuiLib::CButtonUI *official_logout_btn_ = nullptr;

	DuiLib::CComboUI* craft_servers_cb_ = nullptr;
	DuiLib::CComboUI* craft_players_cb_ = nullptr;
	DuiLib::CTabLayoutUI *official_login_tab_ = nullptr;


	DuiLib::CButtonUI *select_java_btn_ = nullptr;
	DuiLib::CButtonUI *java_operator_btn_ = nullptr;
	DuiLib::CLabelUI *java_version_label_ = nullptr;
	DuiLib::CLabelUI *java_searching_tips_ = nullptr;
	DuiLib::CEditUI *java_max_avail_mem_size_edit_ = nullptr;
	DuiLib::CHorizontalLayoutUI *java_dl_layout_ = nullptr;
	DuiLib::CHorizontalLayoutUI *java_download_tips_layout_ = nullptr;
	DuiLib::CProgressUI *java_dl_progress_ = nullptr;
	DuiLib::CButtonUI *cancel_java_dl_btn_ = nullptr;

	DuiLib::CButtonUI *close_btn_ = nullptr;
	DuiLib::CButtonUI *ok_btn_ = nullptr;

	std::shared_ptr<JavaEnvironment> java_env_;
	bool show_combo_ = false;
	UINT_PTR update_dl_progress_timer_ = 0;
};
