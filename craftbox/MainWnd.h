#pragma once

#include "Duilib/UIlib.h"
#include "minecraft_common/OfficialMinecraftVersions.h"
#include "minecraft_common/MinecraftProfile.h"
#include "minecraft_common/MinecraftInstanceManager.h"
#include "minecraft_common/MinecraftInstanceSearch.h"
#include "minecraft_common/IntergrationMinecraftVersions.h"
#include "base/HttpDownloadToFileWithCache.h"
#include "base/HttpResumableDownloadToFile.h"
#include "ConfigWindow.h"
#include "CraftBoxPageEventHandler.h"
#include "BoxUserInfoRequest.h"
#include "DownloadingItemController.h"
#include "UserJoinedServerInfo.h"
#include "CraftBoxTokenFetcher.h"
#include "WebPageWnd.h"
#include "MinecraftProcessInfo.h"

#include <memory>
#include <set>
#include <map>

class ConfigWindow;
class ShellNotifyIcon;

using namespace DuiLib;

#define theMainWnd	(MainWnd::global())

class MainWnd 
	: public WindowImplBase
	, public IListCallbackUI
	, public sigslot::has_slots<>
	, public std::enable_shared_from_this<MainWnd>
{
public:
	static MainWnd* global();
	MainWnd();
	virtual ~MainWnd();

	void Notify(TNotifyUI& msg) override;
	virtual void OnFinalMessage(HWND) override;
	virtual CDuiString GetSkinFile() override;
	virtual CDuiString GetSkinFolder() override;
	virtual LPCTSTR GetWindowClassName(void) const override;
	virtual void InitWindow();
	virtual UILIB_RESOURCETYPE GetResourceType() const override;
	virtual LPCWSTR GetResourceID() const override;
	virtual LPCTSTR GetItemText(CControlUI* pList, int iItem, int iSubItem) override;
	virtual void OnClick(TNotifyUI& msg) override;
	virtual LRESULT OnClose(UINT, WPARAM, LPARAM, BOOL& bHandled) override;
	virtual CControlUI* CreateControl(LPCTSTR pstrClass) override;

	void save_all_data();

	void handle_command_line(LPCWSTR cmd, bool from_startup = true);

	void on_new_web_page(std::wstring url, int width, int height, bool lock);
	
protected:
	void login_with_cookie(const std::wstring &cookie);
	void login_with_cookie(const std::string &cookie);
	void show_user_center_page();
	void show_quit_confirm_window();
	void on_really_close();
	void on_user_header_pic_fetch_state_changed(std::shared_ptr<NetworkTask> task, NetworkTask::State state);
	void refresh_official_game_list(bool only_show_release);
	void on_local_game_record_load_finished();
	void show_config_wnd(ConfigWindow::ConfigWndTab tab = ConfigWindow::kGameSetting, bool async = false);

	void on_java_record_load_finished();
	void on_fetching_user_info();
	void on_fetch_user_info_finished(bool success);
	void on_logout();

	void on_modify_header(const std::string &header_url);
	void on_login_success(bool auto_save);
	void save_cookie(const std::string &cookie);
	void load_cookie(std::string &cookie);
	void launch_game(const std::wstring &base_dir, const std::wstring &version_str);
	void launch_server(std::wstring sid, std::wstring server, int port, std::wstring server_name, std::wstring player_name, int account_type, std::string cmd, bool mod_server, std::vector<std::wstring> client_vers);
	void on_download_server_integrate_pack(std::wstring url, std::wstring server_name, std::string sha1, uint64_t sz);
	virtual LRESULT HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) override;
	virtual LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnNotifyIcon(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	LRESULT OnCopyData(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
	virtual LRESULT OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) override;
	virtual LRESULT ResponseDefaultKeyEvent(WPARAM wParam) override;

	void show_notify_icon_menu(POINT *pt);

	
	void on_load_yggdraisl_account_finished(const std::string &access_token, const std::string &client_token);

	void ui_select_local_game_tab();
	void ui_select_official_game_tab();
	void ui_select_intergrate_game_tab();

	void ui_select_downloading_game_tab();
	void ui_select_downloaded_game_tab();

	void on_official_game_list_updated(bool success);
	void on_integrate_game_list_updated(bool success);

	void on_mc_instances_added(std::shared_ptr<MinecraftInstanceManager::MinecraftInstanceItem> item);
	void on_mc_instances_removed(std::shared_ptr<MinecraftInstanceManager::MinecraftInstanceItem> item);
	void on_mc_instances_all_revoced();
	void on_official_downloading_instance_added(std::shared_ptr<MinecraftDownload> dl_inst);
	void on_intergration_downloading_instance_added(std::shared_ptr<HttpResumableDownloadToFile> dl_inst, std::string file_sha1, uint64_t file_size, std::wstring file_save_path, std::wstring verstion_str);
	void on_download_item_state_changed(std::shared_ptr<DownloadingItemController> item, DownloadingItemController::State state);
	void ui_update_downloading_progress();

	void on_mc_instances_search_begined();
	void on_mc_instances_search_finished(bool search_exe_dir_only);

	void init_mc_instance_search_obj();

	void on_notify_icon_menu_clicked(DuiLib::TNotifyUI &msg);
	void manual_add_mc_directory();

	void on_java_download_failed();
	void on_java_download_success();

	void on_fetch_user_join_server_info_finished();

	bool verify_offline_user_name(const std::wstring &player_name);

	void on_wb_command_state_changed(CraftBoxPageEventHandler *obj);

	void on_web_page_destroyed(WebPageWnd* wnd);

	void delay_save_all_data();

	void add_server_list_history(const std::wstring &java_path, const std::wstring &servers_dat_path, const std::wstring &server_name_and_ips);

	static std::wstring MinecraftServerListModifyJarPath();

	void on_game_crashed(std::shared_ptr<MinecraftProcessInfo> process_info);

private:
	CListUI* local_game_list_ = nullptr;
	CListUI* official_game_list_ = nullptr;
	CListUI* integrate_game_list_ = nullptr;
	CTabLayoutUI* game_list_ = nullptr;
	CTabLayoutUI* main_page_ = nullptr;
	CButtonUI *close_btn_ = nullptr;
	CButtonUI *min_btn_ = nullptr;
	CButtonUI *setting_btn_ = nullptr;

	CButtonUI *backward_btn_ = nullptr;
	CButtonUI *forward_btn_ = nullptr;
	CButtonUI *refresh_btn_ = nullptr;
	CHorizontalLayoutUI *wb_navigator_layout_ = nullptr;

	CButtonUI *official_game_tab_btn_ = nullptr;
	CButtonUI *local_game_tab_btn_ = nullptr;
	CButtonUI *intergrate_game_tab_btn_ = nullptr;
	CLabelUI *local_game_tab_underground_ = nullptr;
	CLabelUI *official_game_tab_underground_ = nullptr;
	CLabelUI *intergrate_game_tab_underground_ = nullptr;
	CButtonUI *login_btn_ = nullptr;
	CButtonUI *user_header_btn_ = nullptr;

	COptionUI *main_page_opt_ = nullptr;
	COptionUI *client_page_opt_ = nullptr;
	COptionUI *server_page_opt_ = nullptr;
	COptionUI *download_page_opt_ = nullptr;
	CGifAnimUI *downloading_ani_ = nullptr;

	CButtonUI *search_game_btn_ = nullptr;
	CLabelUI *search_tips_label_ = nullptr;
	CGifAnimUI *search_ani_ = nullptr;
	CLabelUI *download_tips_label_ = nullptr;
	CButtonUI *external_dl_btn_ = nullptr;
	COptionUI *only_show_release_type_ = nullptr;


	CButtonUI *select_game_btn_ = nullptr;
	CButtonUI *launch_game_btn_ = nullptr;

	CButtonUI *feedback_btn_ = nullptr;
	CLabelUI *product_version_label = nullptr;

	CButtonUI *downloading_game_tab_btn_ = nullptr;
	CButtonUI *downloaded_game_tab_btn_ = nullptr;
	CLabelUI *downloading_tab_underground_ = nullptr;
	CLabelUI *downloaded_tab_underground_ = nullptr;
	CTabLayoutUI *download_tab_layout_ = nullptr;
	CListUI *downloading_list_ = nullptr;
	CListUI *downloaded_list_ = nullptr;

	CEditUI *search_server_edit_ = nullptr;
	CButtonUI *search_server_btn_ = nullptr;

	CWebBrowserUI *main_page_wb_ = nullptr;
	CWebBrowserUI *server_page_wb_ = nullptr;
	CWebBrowserUI *user_page_wb_ = nullptr;

	std::shared_ptr<CraftBoxPageEventHandler> main_page_event_handler_ = std::make_shared<CraftBoxPageEventHandler>();
	std::shared_ptr<CraftBoxPageEventHandler> server_page_event_handler_ = std::make_shared<CraftBoxPageEventHandler>();
	std::shared_ptr<CraftBoxPageEventHandler> user_center_event_handler_ = std::make_shared<CraftBoxPageEventHandler>();

	CDialogBuilder official_game_list_Builder_;
	CDialogBuilder local_game_list_builder_;
	CDialogBuilder integrate_game_list_builder_;
	CDialogBuilder donwloading_list_builder_;

	std::shared_ptr<OfficialMinecraftVersions> official_mc_vers_;
	std::shared_ptr<IntegrationMinecraftVersions> integrate_mc_vers_;
	std::shared_ptr<MinecraftInstanceSearch> mc_inst_search_;
	std::map<std::wstring, std::map<std::wstring, std::shared_ptr<MinecraftProfile>>> downloading_insts_;
	MinecraftInstanceManager *mc_inst_manager_ = nullptr;
	std::shared_ptr<ConfigWindow> config_wnd_;

	UINT_PTR downloading_list_refresh_timer_ = 0;

	struct DownloadItemAndAssociateUI
	{
		std::shared_ptr<DownloadingItemController> item;
		std::wstring version;
		CLabelUI* progress_label = nullptr;
		CButtonUI *cancel_btn = nullptr;
		CButtonUI *pause_btn = nullptr;
		CButtonUI *start_btn = nullptr;
		CButtonUI *retry_btn = nullptr;
		CListContainerElementUI* list_item = nullptr;
	};
	std::map<DownloadingItemController*, std::shared_ptr<DownloadItemAndAssociateUI>> dl_info_map_;

	std::shared_ptr<ShellNotifyIcon> notify_icon_;
	bool need_really_quit_ = false;

	std::thread local_game_record_load_thread_;
	std::thread download_record_load_thread_;
	std::thread yggdrasil_info_load_thread_;
	std::thread java_record_load_thread_;
	std::thread integration_game_record_load_thread_;

	std::wstring default_header_pic_;

	std::map<WebPageWnd*, std::shared_ptr<WebPageWnd>> page_wnds_;

	bool first_show_server_page_ = true;
	bool first_show_user_page_ = true;

	static MainWnd* global_;
};
