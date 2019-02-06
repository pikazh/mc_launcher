#include "CraftBoxHelper.h"

#include <shlobj.h>

JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void *reserved)
{
	::OutputDebugStringA("JNI_OnLoad");

	return JNI_VERSION_1_2;
}

JNIEXPORT void JNICALL
JNI_OnUnload(JavaVM *vm, void *reserved)
{
}

#ifdef __cplusplus
extern "C" {
#endif

	/*
	* Class:     com_craftbox_CraftBoxHelper
	* Method:    onCreateHelper
	* Signature: ()V
	*/
	JNIEXPORT void JNICALL Java_CraftBoxHelper_onCreateHelper
	(JNIEnv *env, jobject obj)
	{
		CraftBoxHelper::instance()->init_with_jni_object(env, obj);
	}

	/*
	* Class:     com_craftbox_CraftBoxHelper
	* Method:    onJoinGame
	* Signature: ()V
	*/
	JNIEXPORT void JNICALL Java_CraftBoxHelper_onJoinGame
	(JNIEnv *, jobject)
	{
		CraftBoxHelper::instance()->on_join_game();
	}

	/*
	* Class:     com_craftbox_CraftBoxHelper
	* Method:    onDisconnectGame
	* Signature: ()V
	*/
	JNIEXPORT void JNICALL Java_CraftBoxHelper_onDisconnectGame
	(JNIEnv *, jobject)
	{
		CraftBoxHelper::instance()->on_disconnect_game();
	}

#ifdef __cplusplus
}
#endif

CraftBoxHelper* CraftBoxHelper::instance()
{
	static CraftBoxHelper *inst = new CraftBoxHelper();
	return inst;
}

CraftBoxHelper::CraftBoxHelper()
{
	::CoInitialize(0);

	help_obj_ = NULL;
	env_ = NULL;

	ipc_wnd_ = new IPCWindow(&CraftBoxHelper::recv_ipc_msg, &CraftBoxHelper::timer_func);

	if (!ipc_wnd_->init())
	{
		::OutputDebugStringA("could not create ipc wnd");
	}
}

CraftBoxHelper::~CraftBoxHelper()
{
	delete ipc_wnd_;
	ipc_wnd_ = NULL;
}

void CraftBoxHelper::init_with_jni_object(JNIEnv *env, jobject obj)
{
	env_ = env;
	help_obj_ = env_->NewGlobalRef(obj);
}

void CraftBoxHelper::on_join_game()
{
	ipc_wnd_->beginTimer(1, 3000);
	//AsyncTaskDispatch::main_thread()->post_delay_task(make_closure(&CraftBoxHelper::on_join_game_impl, this->shared_from_this()), false, 2000);
	//AsyncTaskDispatch::main_thread()->post_delay_task(make_closure(&CraftBoxHelper::on_join_game_impl, this->shared_from_this()), false, 4000);
}

void CraftBoxHelper::on_disconnect_game()
{
	//AsyncTaskDispatch::main_thread()->post_task(make_closure(&CraftBoxHelper::on_disconnect_game_impl, this->shared_from_this()), false);
}

void CraftBoxHelper::on_join_game_impl()
{
	if (mc_chat_msg_.empty())
		return;

	jclass cls = env_->GetObjectClass(help_obj_);
	//DCHECK(cls);
	jmethodID is_single_play_id = env_->GetMethodID(cls, "isSinglePlay", "()Z");
 	//DCHECK(is_single_play_id);
	if (!env_->CallBooleanMethod(help_obj_, is_single_play_id))
	{
		jstring jStringParam = env_->NewStringUTF(mc_chat_msg_.c_str());
		//DCHECK(jStringParam);

		jmethodID send_chat_message_id = env_->GetMethodID(cls, "sendChatMessage", "(Ljava/lang/String;)V");
		//DCHECK(send_chat_message_id);

		env_->CallVoidMethod(help_obj_, send_chat_message_id, jStringParam);

		env_->DeleteLocalRef(jStringParam);
	}
}

void CraftBoxHelper::on_disconnect_game_impl()
{
	//env_->NewGlobalRef()
}

void CraftBoxHelper::on_recv_ipc_msg(IPCDATA &msg)
{
	if (msg.msg == IPC_MSG_LOGIN)
	{
		mc_chat_msg_.assign(msg.data.c_str(), msg.data.length());
		::OutputDebugStringA(mc_chat_msg_.c_str());
	}
	
}

void CraftBoxHelper::recv_ipc_msg(IPCDATA &msg)
{
	CraftBoxHelper::instance()->on_recv_ipc_msg(msg);
}

void CraftBoxHelper::timer_func(UINT_PTR id)
{
	CraftBoxHelper::instance()->on_timer_func(id);
}

void CraftBoxHelper::on_timer_func(UINT_PTR id)
{
	if(1 == id)
	{
		on_join_game_impl();
	}
}
