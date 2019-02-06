#include "JavaComboMenu.h"
#include "JavaEnvironment.h"
#include "glog/logging.h"

JavaComboMenu::JavaComboMenu()
{
}

JavaComboMenu::~JavaComboMenu()
{
}

LRESULT JavaComboMenu::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	__super::OnCreate(uMsg, wParam, lParam, bHandled);

	auto javas = JavaEnvironment::GetInstance()->java_paths_and_vers();
	if (javas.empty())
	{
		Close();
		return 0;
	}
	else
	{
		DuiLib::CListUI* list = static_cast<DuiLib::CListUI*>(GetPaintManager()->FindControl(L"java_combo_list"));
		if (list == nullptr)
		{
			Close();
			return 0;
		}

		RECT rcWnd;
		GetWindowRect(m_hWnd, &rcWnd);
		auto height = rcWnd.bottom - rcWnd.top;
		DuiLib::CDialogBuilder builder;

		int item_count = 0;
		for (auto it = javas.begin(); it != javas.end(); ++it)
		{

			DuiLib::CListContainerElementUI* pListElement = nullptr;
			if (!builder.GetMarkup()->IsValid())
			{
				pListElement = static_cast<DuiLib::CListContainerElementUI*>(builder.Create(_T("JavaComboItem.xml"), (UINT)0, nullptr, GetPaintManager()));
			}
			else
			{
				pListElement = static_cast<DuiLib::CListContainerElementUI*>(builder.Create((UINT)0, GetPaintManager()));
			}

			if (pListElement)
			{
				item_count++;

				DuiLib::CLabelUI *path_label = static_cast<DuiLib::CLabelUI*>(pListElement->FindSubControl(L"path"));
				if (path_label)
				{
					path_label->SetText(it->first.c_str());
					path_label->SetToolTip(std::wstring(L"Â·¾¶: " + it->first).c_str());
					pListElement->AddCustomAttribute(L"version", std::wstring(it->second.begin(), it->second.end()).c_str());
					pListElement->AddCustomAttribute(L"path", it->first.c_str());
					list->Add(pListElement);
				}
				else
				{
					DCHECK(0);
				}
			}
			else
			{
				DCHECK(0);
			}
		}

		if (item_count == 0)
		{
			Close();
			return 0;
		}
		else
		{
			LONG adjust_height = 6 * 2 + item_count * 35;
			if (adjust_height < height)
			{
				MoveWindow(m_hWnd, rcWnd.left, rcWnd.top, rcWnd.right - rcWnd.left, adjust_height, TRUE);
			}
		}

	}

	return 0;
}

void JavaComboMenu::Notify(DuiLib::TNotifyUI & msg)
{
	if (msg.sType == _T("itemselect"))
	{
		Close();
	}
	else if (msg.sType == _T("itemclick"))
	{
		Owner()->GetManager()->SendNotify(Owner(), L"itemclick", (WPARAM)msg.pSender, 0, false);
	}
}
