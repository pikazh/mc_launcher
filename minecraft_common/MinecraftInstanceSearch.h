#pragma once

#include "base/sigslot.h"

#include <list>
#include <memory>
#include <thread>

class MinecraftInstanceSearch
	: public std::enable_shared_from_this<MinecraftInstanceSearch>
{
public:
	MinecraftInstanceSearch();
	virtual ~MinecraftInstanceSearch();

public:
	bool start_search(bool search_exe_dir_only);
	void stop_search();
	bool is_searching() { return searching_; }

	sigslot::signal0<> search_started;
	sigslot::signal1<bool> search_finished;
	sigslot::signal1<bool> search_canceled;
	
protected:
	void search_proc();
private:

	struct SearchItem
	{
		SearchItem(const std::wstring &path, uint8_t depth)
		{
			this->path = path;
			this->depth = depth;
		}
		SearchItem(const wchar_t *path, uint8_t depth)
		{
			this->path = path;
			this->depth = depth;
		}
		std::wstring path;
		uint8_t depth = 0;
	};


	std::thread thread_;
	bool searching_ = false;
	bool search_exe_dir_only_ = false;
	volatile bool stop_ = true;
};

