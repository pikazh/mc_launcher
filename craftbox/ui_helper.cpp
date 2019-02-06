#include "ui_helper.h"

std::wstring get_ui_text_for_mc_inst(const std::shared_ptr<MinecraftInstanceManager::MinecraftInstanceItem> &item, bool enable_html)
{
	return std::move(get_ui_text_for_mc_inst(item->version_str, item->note, enable_html));
}

std::wstring get_ui_text_for_mc_inst(const std::wstring &version_str, const std::wstring &note, bool enable_html)
{
	if (note.empty())
	{
		return version_str;
	}
	else
	{
		if (enable_html)
		{
			std::wstring text = L"<c #333333>" + note + L"</c><c #888888>(" + version_str + L")</c>";
			return std::move(text);
		}
		else
		{
			std::wstring text = note + L"(" + version_str + L")";
			return std::move(text);
		}
	}

}
