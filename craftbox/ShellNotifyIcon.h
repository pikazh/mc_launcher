#pragma once

#include "base/system_info.h"

#include <stdint.h>
#include <windows.h>
#include <memory>

class Closure;

class ShellNotifyIcon
	: public std::enable_shared_from_this<ShellNotifyIcon>
{
public:
	ShellNotifyIcon(HWND main_wnd, UINT notify_message, HICON icon);
	virtual ~ShellNotifyIcon();

public:
	void show_notify_icon();
	void hide_notify_icon();
	void show_notify_message(const wchar_t *msg, const wchar_t *title, uint32_t show_time = 4000);
private:
	HWND main_wnd_ = 0;
	UINT notify_message_ = 0;
	HICON icon_ = nullptr;

	std::shared_ptr<Closure> hide_message_task_;
	SystemInfo::WindowsVersion sys_ver_;
};

