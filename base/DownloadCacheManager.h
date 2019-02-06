#pragma once

#include <string>
#include <mutex>

class DownloadCacheManager
{
public:

	static DownloadCacheManager* GetInstance();
	DownloadCacheManager();
	virtual ~DownloadCacheManager();

public:
	std::wstring cache_entry_for_url(const std::string &url);
	std::string get_if_modified_since(const std::wstring &entry);
	bool update_cache_file(const std::wstring &entry, FILE *srcfp, const std::string &if_modify_since);
	bool update_cache_file(const std::wstring &entry, const std::string &buffer, const std::string &if_modify_since);
	bool read_cache_file(const std::wstring &entry, FILE *dstfp);
	bool read_cache_file(const std::wstring &entry, std::string &buffer);
private:
	std::mutex lock_;
};

