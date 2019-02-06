#include "WebPageWnd.h"
#include "glog/logging.h"
#include "base/asynchronous_task_dispatch.h"
#include "MainWnd.h"

void WebPageWnd::navigate(const wchar_t *url)
{
	wb_->Navigate2(url);
}

void WebPageWnd::OnFinalMessage(HWND hWnd)
{
	__super::OnFinalMessage(hWnd);

	std::weak_ptr<WebPageWnd> weak_obj = this->shared_from_this();
	AsyncTaskDispatch::current()->post_task(make_closure([weak_obj]()
	{
		auto obj = weak_obj.lock();
		if (!!obj)
			obj->destroyed(obj.get());
	}), true);
}

void WebPageWnd::InitWindow()
{
	event_handler_ = std::make_shared<CraftBoxPageEventHandler>();
	event_handler_->new_web_page.connect(theMainWnd, &MainWnd::on_new_web_page);
	event_handler_->title_changed.connect(this, [this](std::wstring title)
	{
		HWND wnd = this->GetHWND();
		if (::IsWindow(wnd))
		{
			::SetWindowText(wnd, title.c_str());
		}
	});
	wb_ = (DuiLib::CWebBrowserUI*)(GetPaintManager()->FindControl(L"wb"));
	wb_->SetDelayCreate(false);
	wb_->SetWebBrowserEventHandler(event_handler_.get());
	CHECK(wb_);
}
