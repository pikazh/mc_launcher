#pragma once

#include "CraftBoxPageEventHandler.h"
#include "DuiLib/UIlib.h"
#include "BaseWindow.h"
#include "base/sigslot.h"

#include <memory>

class WebPageWnd
	: public BaseWindow
	, public std::enable_shared_from_this<WebPageWnd>
{
public:
	WebPageWnd()
		: BaseWindow(L"webpage.xml")
	{

	}

	~WebPageWnd()
	{

	}

	void navigate(const wchar_t *url);

	sigslot::signal1<WebPageWnd*> destroyed;
protected:
	BEGIN_BASE_MSG_MAP
	CHAIN_BASE_MSG_MAP(BaseWindow)

	virtual void OnFinalMessage(HWND hWnd) override;
	virtual void InitWindow() override;
private:
	std::shared_ptr<CraftBoxPageEventHandler> event_handler_;
	DuiLib::CWebBrowserUI* wb_ = nullptr;
};
