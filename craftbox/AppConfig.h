#pragma once

#include <string>
#include <map>

class AppConfig
{
public:
	static AppConfig* global();
	AppConfig();
	virtual ~AppConfig();

public:
	void load();
	void save();

	void set_value_for_key(const std::wstring &key, const std::wstring &value);
	std::wstring get_value_for_key(const std::wstring &key);

protected:
	void unsafe_load();
private:
	std::map<std::wstring, std::wstring> stores_;
};

