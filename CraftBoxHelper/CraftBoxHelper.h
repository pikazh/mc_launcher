#pragma once

#include "jni.h"
#include "base/asynchronous_task_dispatch.h"
#include "IPCWindow/IPCWindow.h"
#include "base/sigslot.h"

#include <windows.h>
#include <string>
#include <memory>

class CraftBoxHelper
	: public std::enable_shared_from_this<CraftBoxHelper>
	, public sigslot::has_slots<>
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

	void on_recv_ipc_msg(std::shared_ptr<IPCDATA> msg);
private:
	jobject help_obj_ = nullptr;
	JNIEnv *env_ = nullptr;
	std::shared_ptr<IPCWindow> ipc_wnd_;
	std::string mc_chat_msg_;
};
