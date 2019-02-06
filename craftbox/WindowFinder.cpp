#include "WindowFinder.h"



WindowFinder::WindowFinder()
{
}


WindowFinder::~WindowFinder()
{
}

void WindowFinder::set_process_id(DWORD id)
{
	process_id_ = id;
}

void WindowFinder::set_window_class_name(LPCWSTR class_name)
{
	class_name_ = class_name;
}

HWND WindowFinder::find()
{
	find_result = (HWND)-1;
	EnumWindows(&WindowFinder::find_window_callback, (LPARAM)this);
	return find_result;
}

BOOL WindowFinder::find_window_callback(HWND wnd, LPARAM param)
{
	WindowFinder *pthis = (WindowFinder*)(param);

	if (!pthis->class_name_.empty())
	{
		wchar_t class_name[128] = { 0 };
		GetClassNameW(wnd, class_name, 127);
		if (pthis->class_name_.compare(class_name) != 0)
			return TRUE;
	}

	if (pthis->process_id_ != 0)
	{
		DWORD pid = 0;
		::GetWindowThreadProcessId(wnd, &pid);
		if (pid != pthis->process_id_)
			return TRUE;
	}

	pthis->find_result = wnd;
	return FALSE;
}
