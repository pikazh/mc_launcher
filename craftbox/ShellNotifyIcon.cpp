#include "ShellNotifyIcon.h"
#include "base/asynchronous_task_dispatch.h"
#include "minecraft_common/MinecraftCommon.h"

ShellNotifyIcon::ShellNotifyIcon(HWND main_wnd, UINT notify_message, HICON icon)
{
	main_wnd_ = main_wnd;
	notify_message_ = notify_message;
	icon_ = icon;

	sys_ver_ = SystemInfo().windows_version();
}


ShellNotifyIcon::~ShellNotifyIcon()
{
}

void ShellNotifyIcon::show_notify_icon()
{
	NOTIFYICONDATAW nid = { 0 };
	if (sys_ver_ >= SystemInfo::WINDOWS_7)
	{
		nid.cbSize = sizeof(NOTIFYICONDATAW);
	}
	else
	{
		nid.uID = 100;
		nid.cbSize = NOTIFYICONDATAW_V3_SIZE;
	}
	nid.hWnd = main_wnd_;
	nid.hIcon = icon_;
	wcscpy(nid.szTip, mcfe::g_app_name);
	nid.uVersion = sys_ver_ >= SystemInfo::WINDOWS_7? NOTIFYICON_VERSION_4:NOTIFYICON_VERSION;
	nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
		
	nid.uCallbackMessage = notify_message_;
	Shell_NotifyIconW(NIM_MODIFY, &nid);
	Shell_NotifyIconW(NIM_ADD, &nid);
	
}

void ShellNotifyIcon::hide_notify_icon()
{
	NOTIFYICONDATAW nid = { 0 };
	if (sys_ver_ >= SystemInfo::WINDOWS_7)
	{
		nid.cbSize = sizeof(NOTIFYICONDATAW);
	}
	else
	{
		nid.uID = 100;
		nid.cbSize = NOTIFYICONDATAW_V3_SIZE;
	}
	nid.hWnd = main_wnd_;
	nid.hIcon = icon_;
	nid.uVersion = sys_ver_ >= SystemInfo::WINDOWS_7 ? NOTIFYICON_VERSION_4 : NOTIFYICON_VERSION;
	nid.uFlags = NIF_ICON;

	Shell_NotifyIcon(NIM_DELETE, &nid);
}

void ShellNotifyIcon::show_notify_message(const wchar_t *msg, const wchar_t *title, uint32_t show_time)
{
	NOTIFYICONDATAW nid = { 0 };
	if (sys_ver_ >= SystemInfo::WINDOWS_7)
	{
		nid.cbSize = sizeof(NOTIFYICONDATAW);
	}
	else
	{
		nid.uID = 100;
		nid.cbSize = NOTIFYICONDATAW_V3_SIZE;
	}
	nid.hWnd = main_wnd_;
	nid.uVersion = sys_ver_ >= SystemInfo::WINDOWS_7 ? NOTIFYICON_VERSION_4 : NOTIFYICON_VERSION;
	nid.hIcon = icon_;

	wcscpy(nid.szTip, mcfe::g_app_name);
	nid.uVersion = NOTIFYICON_VERSION;
	nid.uFlags = NIF_ICON | NIF_TIP | NIF_INFO;
	if (sys_ver_ >= SystemInfo::WINDOWS_7)
	{
		nid.hBalloonIcon = icon_;
	}
		
	wcscpy(nid.szInfo, msg);
	wcscpy(nid.szInfoTitle, title);
	nid.dwInfoFlags = NIIF_USER | NIIF_NOSOUND;
	Shell_NotifyIconW(NIM_MODIFY, &nid);

	if (!!hide_message_task_)
	{
		hide_message_task_->cancel();
	}

	if (show_time != 0)
	{
		hide_message_task_ = make_closure(&ShellNotifyIcon::show_notify_message, this->shared_from_this(), L"", L"", 0);
		AsyncTaskDispatch::current()->post_delay_task(hide_message_task_, true, show_time);
	}
	
}
