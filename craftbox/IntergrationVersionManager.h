#pragma once
#include "base/HttpResumableDownloadToFile.h"

#include <map>
#include <mutex>
#include <string>
#include <stdint.h>

class IntergrationVersionManager
	: public std::enable_shared_from_this<IntergrationVersionManager>
{
public:
	static IntergrationVersionManager* global();
	
	IntergrationVersionManager();
	virtual ~IntergrationVersionManager();

public:
	sigslot::signal5<std::shared_ptr<HttpResumableDownloadToFile>, std::string, uint64_t, std::wstring, std::wstring> download_inst_added;
	std::map<std::string, std::list<std::tuple<std::shared_ptr<HttpResumableDownloadToFile>, std::string, uint64_t, std::wstring, std::wstring>>> downloading_instances();
	std::shared_ptr<HttpResumableDownloadToFile> download_instance(const std::string &url, const std::wstring &download_path, const std::string &sha1, uint64_t size, std::wstring extrace_path, std::wstring version_str, bool *is_exist_before = nullptr);
	void remove_download_instance(std::shared_ptr<HttpResumableDownloadToFile> inst);
private:
	std::map<std::string, std::list<std::tuple<std::shared_ptr<HttpResumableDownloadToFile>, std::string, uint64_t, std::wstring, std::wstring>>> downloading_insts_;
	std::recursive_mutex lock_;
};
