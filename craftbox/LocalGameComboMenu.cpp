#include "LocalGameComboMenu.h"
#include "minecraft_common/MinecraftInstanceManager.h"
#include "glog/logging.h"
#include "ui_helper.h"

LocalGameComboMenu::LocalGameComboMenu()
{
}

LocalGameComboMenu::~LocalGameComboMenu()
{
}

LRESULT LocalGameComboMenu::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
	__super::OnCreate(uMsg, wParam, lParam, bHandled);

	auto insts = MinecraftInstanceManager::GetInstance()->local_instances();
	if (insts.empty())
	{
		Close();
		return 0;
	}
	else
	{
		DuiLib::CListUI* list = static_cast<DuiLib::CListUI*>(GetPaintManager()->FindControl(L"local_game_combo_list"));
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
		for (auto it = insts.begin(); it != insts.end(); ++it)
		{
			for (auto obj = it->second.begin(); obj != it->second.end(); ++obj)
			{
				DuiLib::CListContainerElementUI* pListElement = nullptr;
				if (!builder.GetMarkup()->IsValid())
				{
					pListElement = static_cast<DuiLib::CListContainerElementUI*>(builder.Create(_T("LocalGameComboItem.xml"), (UINT)0, nullptr, GetPaintManager()));
				}
				else
				{
					pListElement = static_cast<DuiLib::CListContainerElementUI*>(builder.Create((UINT)0, GetPaintManager()));
				}

				if (pListElement)
				{
					item_count++;

					DuiLib::CLabelUI *version_label = static_cast<DuiLib::CLabelUI*>(pListElement->FindSubControl(L"version"));
					if (version_label)
					{
						version_label->SetShowHtml(true);
						version_label->SetText(get_ui_text_for_mc_inst(obj->second).c_str());
						version_label->SetToolTip(std::wstring(L"Â·¾¶: " + obj->second->base_dir + L"\\versions\\" + obj->second->version_str).c_str());

						pListElement->AddCustomAttribute(L"base_dir", obj->second->base_dir.c_str());
						pListElement->AddCustomAttribute(L"version_str", obj->second->version_str.c_str());
						pListElement->AddCustomAttribute(L"note", obj->second->note.c_str());

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
			
		}

		if (item_count == 0)
		{
			Close();
			return 0;
		}
		else
		{
			LONG adjust_height = 12+ item_count * 24;
			if (adjust_height < height)
			{
				MoveWindow(m_hWnd, rcWnd.left, rcWnd.top, rcWnd.right - rcWnd.left, adjust_height, TRUE);
			}
		}

	}

	return 0;
}

void LocalGameComboMenu::Notify(DuiLib::TNotifyUI & msg)
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
