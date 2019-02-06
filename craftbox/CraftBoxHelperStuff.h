#pragma once

#include <string>
#include <map>
#include <memory>
#include <mutex>

class CraftBoxHelperStuff
	: public std::enable_shared_from_this<CraftBoxHelperStuff>
{
public:
	CraftBoxHelperStuff();
	virtual ~CraftBoxHelperStuff();
	static CraftBoxHelperStuff *global();

	bool replace(const std::wstring &java_path, const std::wstring &client_jar_path, std::wstring &result);
	void add_mod(const std::wstring &java_path, const std::wstring &client_jar_path, const std::wstring &mod_dir);
	bool check_version(const std::wstring &java_path, const std::wstring &client_jar_path, bool mod_base, std::string &version);
protected:
	static std::wstring extrace_mccraft_zip();
	static std::wstring extrace_mccraft_mod_zip();
	bool found_replaced_jar_in_cache(const std::string &ver, const std::wstring &client_jar_path, std::wstring &replaced_jar_path);
	void set_replaced_jar_cache(const std::string &ver, const std::wstring &client_jar_path, const std::wstring &replaced_jar_path);
private:
	std::map<std::string, std::map<std::wstring, std::wstring>> replaced_java_clients_;
	std::recursive_mutex lock_;
};

