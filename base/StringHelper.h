#pragma once

#include <string>
#include <vector>
#include <windows.h>
#include <Strsafe.h>

inline void wstring_trim_right(std::wstring &str, wchar_t ch)
{
	if (str.empty())
		return;

	auto index = str.size();
	while (str[--index] == ch);
	if (index != str.size() - 1)
	{
		str = str.substr(0, index + 1);
	}
}

inline void wstring_trim_left(std::wstring &str, wchar_t ch)
{
	if (str.empty())
		return;

	auto index = -1;
	while (str[++index] == ch);
	if (index != 0)
	{
		str = str.substr(index);
	}
}

inline void wstring_trim(std::wstring &str, wchar_t ch)
{
	wstring_trim_left(str, ch);
	wstring_trim_right(str, ch);
}

inline void wstring_replace_all(std::wstring &str, const wchar_t *old_char, const wchar_t *new_char)
{
	auto old_len = wcslen(old_char);
	auto new_len = wcslen(new_char);
	auto index = str.find(old_char, 0);
	while (index != std::wstring::npos)
	{
		str.replace(index, old_len, new_char);
		index = str.find(old_char, index+new_len);
	}
}

inline void guid_wstring(std::wstring &guid_string)
{
	GUID guid;
	if (S_OK == ::CoCreateGuid(&guid))
	{
		wchar_t buf[1024] = { 0 };
		StringCchPrintfW(buf, _countof(buf)
			, L"{%08X-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X}"
			, guid.Data1
			, guid.Data2
			, guid.Data3
			, guid.Data4[0]
			, guid.Data4[1]
			, guid.Data4[2]
			, guid.Data4[3]
			, guid.Data4[4]
			, guid.Data4[5]
			, guid.Data4[6]
			, guid.Data4[7]
			);

		guid_string = buf;
	}	
}

inline bool wstring_is_abs_path(const std::wstring &path)
{
	if (path.length() < 2)
		return false;
	
	{
		auto &ch = path.at(1);
		if (ch != L':')
			return false;
	}
	
	{
		auto &ch = path.at(0);
		if (ch < L'c' && ch > L'z' && ch<L'A' && ch>L'Z')
			return false;
	}
	
	for (auto it = path.begin()+2; it != path.end(); ++it)
	{
		if (*it == L':'
			|| *it == L'*'
			|| *it == L'?'
			|| *it == L'"'
			|| *it == L'<'
			|| *it == L'>'
			|| *it == L'|'
			)
			return false;
	}

	return true;
}

inline void wstring_path_remove_duplicated_separator(std::wstring &path)
{
	wstring_replace_all(path, L"/", L"\\");
	wstring_trim_right(path, L'\\');
	bool found_separator = false;
	for (auto it = path.begin(); it != path.end(); )
	{
		if (found_separator)
		{
			if (*it == '\\')
			{
				it = path.erase(it);
				continue;
			}
			else
			{
				found_separator = false;
			}
		}
		else
		{
			if (*it == '\\')
				found_separator = true;
		}
		
		++it;
	}
}

inline std::vector<std::string> split_string(const std::string &str, char* token)
{
	std::vector<std::string> splited_strs;
	std::string new_str(str);
	char* element = strtok((char*)new_str.c_str(), token);
	if (element)
	{
		splited_strs.push_back(element);

		while ((element = strtok(nullptr, token)) != nullptr)
		{
			splited_strs.push_back(element);
		}
	}

	return splited_strs;
}