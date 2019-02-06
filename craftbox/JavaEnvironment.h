#pragma once

#include <string>
#include <map>
#include <memory>
#include <mutex>

#include "base/sigslot.h"

class JavaFinder;
class JavaDownload;

class JavaEnvironment
	: public sigslot::has_slots<>
	, public std::enable_shared_from_this<JavaEnvironment>
{
public:
	static std::shared_ptr<JavaEnvironment> GetInstance();

	JavaEnvironment();
	virtual ~JavaEnvironment();

	std::wstring selected_java_path()
	{
		std::lock_guard<std::recursive_mutex> l(lock_); 
		return select_java_path_; 
	}

	void set_select_java_path(const std::wstring &path)
	{
		std::lock_guard<std::recursive_mutex> l(lock_); 
		select_java_path_ = path; 
	}

	void reset_select_java_path();

	uint32_t java_Xmx_MB() { return java_avail_mem_size_in_mb_; }
	void set_java_Xmx_MB(uint32_t mem_size_in_mb);
	uint32_t default_java_Xmx_MB();
	bool try_add_java(const std::wstring &path, bool set_default = false);

	std::map<std::wstring, std::string> java_paths_and_vers()
	{
		std::lock_guard<std::recursive_mutex> l(lock_);
		return java_paths_and_vers_;
	}

	sigslot::signal2<std::wstring, std::string> java_added;
public:
	void init();
	void save() { save_to_profile_file(); }
	std::shared_ptr<JavaFinder> search_java();
	std::shared_ptr<JavaDownload> download_java();
protected:
	void load_from_profile_file();
	void save_to_profile_file();

	bool insert_java_record(const std::wstring &java_path, const std::string &version);
private:
	std::map<std::wstring, std::string> java_paths_and_vers_;
	std::set<std::wstring> java_path_compare_set_;
	std::shared_ptr<JavaFinder> java_finder_;
	std::shared_ptr<JavaDownload> java_dl_;

	std::wstring select_java_path_;
	uint32_t java_avail_mem_size_in_mb_ = 0;
	bool is_x64_sys_ = false;
	uint32_t sys_mem_size_in_mb_ = 0;
	
	std::recursive_mutex lock_;
};

