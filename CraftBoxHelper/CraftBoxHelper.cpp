#include "CraftBoxHelper.h"
#include "glog/logging.h"
#include "base/PathService.h"

#include <shlobj.h>

JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void *reserved)
{
	auto id = ::GetCurrentThreadId();
	std::wstring id_str = std::to_wstring(id);
	id_str = L"JNI_OnLoad, tid: " + id_str;
	::OutputDebugStringW(id_str.c_str());

	return JNI_VERSION_1_2;
}

JNIEXPORT void JNICALL
JNI_OnUnload(JavaVM *vm, void *reserved)
{
	auto id = ::GetCurrentThreadId();
	std::wstring id_str = std::to_wstring(id);
	id_str = L"JNI_OnUnload, tid: " + id_str;
	::OutputDebugStringW(id_str.c_str());
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
	static std::shared_ptr<CraftBoxHelper> inst = std::make_shared<CraftBoxHelper>();
	return inst.get();
}

CraftBoxHelper::CraftBoxHelper()
{
	::CoInitialize(nullptr);
	google::InitGoogleLogging("craftbox");

#ifdef _DEBUG
	google::SetStderrLogging(google::GLOG_WARNING);
#endif

	std::wstring log_dir = PathService().appdata_path();
	log_dir.append(L"\\log");
	::SHCreateDirectory(0, log_dir.c_str());
	char temp[1024] = { 0 };
	WideCharToMultiByte(CP_ACP, 0, log_dir.c_str(), -1, temp, 1024, NULL, NULL);
	static std::string log_dir_str = temp;
	FLAGS_log_dir = log_dir_str;

	google::SetLogDestination(google::GLOG_WARNING, "");
	google::SetLogDestination(google::GLOG_ERROR, "");
	google::SetLogDestination(google::GLOG_FATAL, "");

	AsyncTaskDispatch::init_main_thread_dispatcher();

	ipc_wnd_ = std::make_shared<IPCWindow>();
	ipc_wnd_->msg_recv.connect(this, &CraftBoxHelper::on_recv_ipc_msg);

	if (!ipc_wnd_->init())
	{
		LOG(ERROR) << "could not create ipc wnd";
	}
}

CraftBoxHelper::~CraftBoxHelper()
{
}

void CraftBoxHelper::init_with_jni_object(JNIEnv *env, jobject obj)
{
	env_ = env;
	help_obj_ = env_->NewGlobalRef(obj);
}

void CraftBoxHelper::on_join_game()
{
	AsyncTaskDispatch::main_thread()->post_delay_task(make_closure(&CraftBoxHelper::on_join_game_impl, this->shared_from_this()), false, 2000);
	AsyncTaskDispatch::main_thread()->post_delay_task(make_closure(&CraftBoxHelper::on_join_game_impl, this->shared_from_this()), false, 4000);
}

void CraftBoxHelper::on_disconnect_game()
{
	AsyncTaskDispatch::main_thread()->post_task(make_closure(&CraftBoxHelper::on_disconnect_game_impl, this->shared_from_this()), false);
}

void CraftBoxHelper::on_join_game_impl()
{
	if (mc_chat_msg_.empty())
		return;

	jclass cls = env_->GetObjectClass(help_obj_);
	DCHECK(cls);
	jmethodID is_single_play_id = env_->GetMethodID(cls, "isSinglePlay", "()Z");
 	DCHECK(is_single_play_id);
	if (!env_->CallBooleanMethod(help_obj_, is_single_play_id))
	{
		jstring jStringParam = env_->NewStringUTF(mc_chat_msg_.c_str());
		DCHECK(jStringParam);

		jmethodID send_chat_message_id = env_->GetMethodID(cls, "sendChatMessage", "(Ljava/lang/String;)V");
		DCHECK(send_chat_message_id);

		env_->CallVoidMethod(help_obj_, send_chat_message_id, jStringParam);

		env_->DeleteLocalRef(jStringParam);
	}
}

void CraftBoxHelper::on_disconnect_game_impl()
{
	//env_->NewGlobalRef()
}

void CraftBoxHelper::on_recv_ipc_msg(std::shared_ptr<IPCDATA> msg)
{
	if (msg->msg == IPC_MSG_LOGIN)
	{
		mc_chat_msg_.assign(msg->data.c_str(), msg->data.length());
		LOG(WARNING) << "recv login cmd: " << mc_chat_msg_;
	}
	
}
