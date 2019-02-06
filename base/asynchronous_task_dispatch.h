#pragma once

#include <windows.h>
#include <memory>
#include <vector>
#include <map>
#include <list>
#include <mutex>

#include "closure.h"

//so far, it bases on win32 message loop, 
//so you'd better not to use in on IO thread. 
class AsyncTaskDispatch:
    public std::enable_shared_from_this<AsyncTaskDispatch>
{
public:
    AsyncTaskDispatch();
    virtual ~AsyncTaskDispatch();

    AsyncTaskDispatch(const AsyncTaskDispatch &) = delete;
    AsyncTaskDispatch& operator=(const AsyncTaskDispatch &) = delete;

    static std::shared_ptr<AsyncTaskDispatch> current();
	static std::shared_ptr<AsyncTaskDispatch> main_thread();
	static void init_main_thread_dispatcher();

protected:

    void init();
    void uninit();

public:
    
    void post_task(std::shared_ptr<Closure> task, bool nestable);
    void post_delay_task(std::shared_ptr<Closure> task, bool nestable, uint32_t delay_time_ms);

protected:
    static LRESULT CALLBACK wnd_msg_proc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);
    static void CALLBACK TimerProc(
        _In_ HWND     hwnd,
        _In_ UINT     uMsg,
        _In_ UINT_PTR idEvent,
        _In_ DWORD    dwTime
        );
    
    void process_non_nestable_task();
	void process_nestable_task();
    void process_delay_task(uint32_t task_key);

private:
    const static uint32_t dispatch_nonnested_msg_id_;
	const static uint32_t dispatch_nestable_msg_id_;
    static std::map<DWORD, std::shared_ptr<AsyncTaskDispatch>> dispatcher_on_each_thread_;
    static std::recursive_mutex lock_for_dispatch_maps_;
	static std::shared_ptr<AsyncTaskDispatch> main_thread_dispatch_;
    std::vector<std::shared_ptr<Closure>> need_dispatch_non_nested_evts_;
	std::list<std::shared_ptr<Closure>> need_dispatch_nestable_evts_;
    std::map<uint32_t, std::shared_ptr<Closure>> delay_evts_;
    uint32_t delay_evt_key_ = 0;
	std::recursive_mutex need_dispatch_evts_lock_;
    HWND win_ = 0;
    DWORD tid_ = 0;
    uint32_t process_enter_count_ = 0;
    bool process_reenter_ = false;
};