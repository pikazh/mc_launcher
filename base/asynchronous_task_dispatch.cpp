#include "asynchronous_task_dispatch.h"
#include "glog/logging.h"

#include <assert.h>

const uint32_t AsyncTaskDispatch::dispatch_nonnested_msg_id_ = ::RegisterWindowMessageW(L"wb_async_non_nested_task_msg");
const uint32_t AsyncTaskDispatch::dispatch_nestable_msg_id_ = ::RegisterWindowMessageW(L"wb_async_nestable_task_msg");
std::map<DWORD, std::shared_ptr<AsyncTaskDispatch>> AsyncTaskDispatch::dispatcher_on_each_thread_;
std::recursive_mutex AsyncTaskDispatch::lock_for_dispatch_maps_;
std::shared_ptr<AsyncTaskDispatch> AsyncTaskDispatch::main_thread_dispatch_;

AsyncTaskDispatch::AsyncTaskDispatch()
{
    init();
}

AsyncTaskDispatch::~AsyncTaskDispatch()
{
    uninit();
}

void AsyncTaskDispatch::init()
{
    tid_ = ::GetCurrentThreadId();
    win_ = ::CreateWindowExW(0, L"#32770", nullptr, 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, nullptr, nullptr);
    assert(win_ != 0);
    if (win_ == 0)
    {
        return;
    }
    SetWindowLongPtrW(win_, GWLP_WNDPROC, (LONG_PTR)&AsyncTaskDispatch::wnd_msg_proc);
    SetWindowLongPtrW(win_, GWLP_USERDATA, (LONG_PTR)this);
}

void AsyncTaskDispatch::uninit()
{
    if (win_ != 0)
    {
        SetWindowLongPtrW(win_, GWLP_USERDATA, 0);
        ::DestroyWindow(win_);

        win_ = 0;
    }

	std::lock_guard<decltype(need_dispatch_evts_lock_)> l(need_dispatch_evts_lock_);
    need_dispatch_nestable_evts_.clear();
	need_dispatch_non_nested_evts_.clear();
}

LRESULT CALLBACK AsyncTaskDispatch::wnd_msg_proc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == dispatch_nonnested_msg_id_)
    {
        LONG_PTR ptr = GetWindowLongPtrW(wnd, GWLP_USERDATA);
        if (ptr)
        {
            AsyncTaskDispatch *pThis = (AsyncTaskDispatch*)ptr;
            pThis->process_enter_count_++;
            if (pThis->process_enter_count_ == 1)
            {
                pThis->process_non_nestable_task();
                if (pThis->process_reenter_)
                {
                    ::PostMessage(wnd, AsyncTaskDispatch::dispatch_nonnested_msg_id_, 0, 0);
                    pThis->process_reenter_ = false;
                }
            }
            else
            {
                pThis->process_reenter_ = true;
            }
            pThis->process_enter_count_--;
        }

    }
	else if (msg == dispatch_nestable_msg_id_)
	{
		LONG_PTR ptr = GetWindowLongPtrW(wnd, GWLP_USERDATA);
		if (ptr)
		{
			AsyncTaskDispatch *pThis = (AsyncTaskDispatch*)ptr;
			pThis->process_nestable_task();
		}
	}

    return 0;
}

void CALLBACK AsyncTaskDispatch::TimerProc(_In_ HWND hwnd, _In_ UINT uMsg, _In_ UINT_PTR idEvent, _In_ DWORD dwTime)
{
	KillTimer(hwnd, idEvent);
    LONG_PTR ptr = GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    if (ptr)
    {
        AsyncTaskDispatch *pThis = (AsyncTaskDispatch*)ptr;
        pThis->process_delay_task(idEvent);
    }
}

void AsyncTaskDispatch::post_task(std::shared_ptr<Closure> task, bool nestable)
{
	std::lock_guard<decltype(need_dispatch_evts_lock_)> l(need_dispatch_evts_lock_);

	if (!nestable)
	{
		if (need_dispatch_non_nested_evts_.empty())
		{
			::PostMessageW(win_, dispatch_nonnested_msg_id_, 0, 0);
		}

		need_dispatch_non_nested_evts_.push_back(task);
	}
	else
	{
		need_dispatch_nestable_evts_.push_back(task);
		::PostMessageW(win_, dispatch_nestable_msg_id_, 0, 0);
	}
}

void AsyncTaskDispatch::post_delay_task(std::shared_ptr<Closure> task, bool nestable, uint32_t delay_time_ms)
{
    if (delay_time_ms == 0)
    {
        post_task(task, nestable);
        return;
    }

    if (::GetCurrentThreadId() == tid_)
    {
        delay_evts_[delay_evt_key_] = task;
        ::SetTimer(win_, delay_evt_key_, delay_time_ms, &AsyncTaskDispatch::TimerProc);
        delay_evt_key_++;
    }
    else
    {
        uint32_t post_time = ::GetTickCount();
        auto func = [this, post_time](std::shared_ptr<Closure> task, uint32_t delay_time_ms)
        {
            uint32_t has_delay = ::GetTickCount() - post_time;
            if (delay_time_ms <= has_delay)
            {
                if (!task->is_canceled())
                    task->run();
            }
            else
            {
                delay_evts_[delay_evt_key_] = task;
                ::SetTimer(win_, delay_evt_key_, delay_time_ms-has_delay, &AsyncTaskDispatch::TimerProc);
                delay_evt_key_++;
            }
            
        };

        post_task(make_closure<decltype(func), std::shared_ptr<Closure>, uint32_t>(func, task, delay_time_ms), nestable);
    }
}

void AsyncTaskDispatch::process_non_nestable_task()
{
    std::vector<std::shared_ptr<Closure>> dispatching_evts;

    {
		std::lock_guard<decltype(need_dispatch_evts_lock_)> l(need_dispatch_evts_lock_);
        dispatching_evts.swap(need_dispatch_non_nested_evts_);
    }

    for (auto& it : dispatching_evts)
    {
        if (!it->is_canceled())
            it->run();
        it.reset();
    }
}

void AsyncTaskDispatch::process_nestable_task()
{
	std::shared_ptr<Closure> task;

	{
		std::lock_guard<decltype(need_dispatch_evts_lock_)> l(need_dispatch_evts_lock_);
		DCHECK(!need_dispatch_nestable_evts_.empty());
		task = need_dispatch_nestable_evts_.front();
		need_dispatch_nestable_evts_.pop_front();
	}

	if (!task->is_canceled())
		task->run();
}

void AsyncTaskDispatch::process_delay_task(uint32_t task_key)
{
    auto it = delay_evts_.find(task_key);
    if (it == delay_evts_.end())
    {
        assert(0);
    }
    else
    {
        auto task = it->second;
        delay_evts_.erase(it);

        if (!task->is_canceled())
            task->run();
    }
}

std::shared_ptr<AsyncTaskDispatch> AsyncTaskDispatch::current()
{
    DWORD tid = ::GetCurrentThreadId();
	std::lock_guard<decltype(lock_for_dispatch_maps_)> l(lock_for_dispatch_maps_);
    auto it = dispatcher_on_each_thread_.find(tid);
    if (it != dispatcher_on_each_thread_.end())
        return it->second;
    else
    {
        auto ptr = std::make_shared<AsyncTaskDispatch>();
        dispatcher_on_each_thread_[tid] = ptr;
        return ptr;
    }
}

std::shared_ptr<AsyncTaskDispatch> AsyncTaskDispatch::main_thread()
{
	return main_thread_dispatch_;
}

void AsyncTaskDispatch::init_main_thread_dispatcher()
{
	main_thread_dispatch_ = current();
}

