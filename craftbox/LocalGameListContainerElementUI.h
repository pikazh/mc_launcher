#pragma once

#include "DuiLib/UIlib.h"
#include "base/sigslot.h"

class LocalGameListContainerElementUI
	: public DuiLib::CListContainerElementUI
{
public:
	LocalGameListContainerElementUI();
	~LocalGameListContainerElementUI();

	virtual LPCTSTR GetClass() const override;

	sigslot::signal0<> mouse_entered;
	sigslot::signal0<> mouse_leaved;

protected:
	virtual void DoEvent(DuiLib::TEventUI& event) override;
};

