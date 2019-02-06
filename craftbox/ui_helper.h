#pragma once

#include "minecraft_common/MinecraftInstanceManager.h"

#include <string>
#include <memory>

#define MSG_PASS_APP_PARAM	(198709)

std::wstring get_ui_text_for_mc_inst(const std::wstring &version_str, const std::wstring &note, bool enable_html = true);
std::wstring get_ui_text_for_mc_inst(const std::shared_ptr<MinecraftInstanceManager::MinecraftInstanceItem> &item, bool enable_html = true);