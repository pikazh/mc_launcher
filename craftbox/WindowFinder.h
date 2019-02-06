#pragma once

#include <windows.h>
#include <string>

class WindowFinder
{
public:
	WindowFinder();
	virtual ~WindowFinder();

	void set_process_id(DWORD id);
	void set_window_class_name(LPCWSTR class_name);

	HWND find();

protected:
	static BOOL CALLBACK find_window_callback(HWND wnd, LPARAM param);
private:
	DWORD process_id_ = 0;
	std::wstring class_name_;
	HWND find_result = (HWND)-1;
};

