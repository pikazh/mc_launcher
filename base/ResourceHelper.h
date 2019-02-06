#pragma once

#include <windows.h>
#include <assert.h>
#include <string>
#include <shlwapi.h>
#include <shlobj.h>

bool extrace_resource(HMODULE module, LPCWSTR resource_name, LPCWSTR resource_type, const std::wstring& path);