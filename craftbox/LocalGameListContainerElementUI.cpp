#include "LocalGameListContainerElementUI.h"

LocalGameListContainerElementUI::LocalGameListContainerElementUI()
{
}

LocalGameListContainerElementUI::~LocalGameListContainerElementUI()
{
}

LPCTSTR LocalGameListContainerElementUI::GetClass() const
{
	return L"LocalGameListContainerElement";
}

void LocalGameListContainerElementUI::DoEvent(DuiLib::TEventUI & event)
{
	if (event.Type == DuiLib::UIEVENT_MOUSEENTER)
	{
		mouse_entered();
	}
	if (event.Type == DuiLib::UIEVENT_MOUSELEAVE)
	{
		mouse_leaved();
	}

	__super::DoEvent(event);
}
