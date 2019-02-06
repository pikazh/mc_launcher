#pragma once

#include <stdint.h>
#include <string>
#include <Windows.h>

class SystemHelper
{
public:
	SystemHelper();
	~SystemHelper();

	bool VerifyEmbeddedSignature(const wchar_t *pwszSourceFile);

	bool WriteClipBoard(HWND hwndowner, const std::wstring &text);
	bool WriteClipBoard(HWND hwndowner, const std::string &text);
};

