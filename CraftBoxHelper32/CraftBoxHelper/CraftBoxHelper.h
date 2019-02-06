#pragma once

#include "jni.h"
#include "IPCWindow.h"

#include <windows.h>
#include <string>

class CraftBoxHelper
{
public:
	static CraftBoxHelper* instance();
	CraftBoxHelper();
	virtual ~CraftBoxHelper();

public:
	void init_with_jni_object(JNIEnv *env, jobject obj);
	void on_join_game();
	void on_disconnect_game();

protected:
	void on_join_game_impl();
	void on_disconnect_game_impl();

	void on_recv_ipc_msg(IPCDATA &msg);
	void on_timer_func(UINT_PTR id);

	static void recv_ipc_msg(IPCDATA &msg);
	static void timer_func(UINT_PTR id);
private:
	jobject help_obj_;
	JNIEnv *env_;
	IPCWindow *ipc_wnd_;
	std::string mc_chat_msg_;
};
