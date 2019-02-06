#pragma once

#include "base/sigslot.h"

#include <thread>
#include <map>
#include <string>
#include <mutex>

class JavaFinder
	: public std::enable_shared_from_this<JavaFinder>
{
public:

	JavaFinder();
	virtual ~JavaFinder();

	bool begin_search();
	void stop_search();
	bool is_searching() {return is_searching_;}

	sigslot::signal0<> search_finished;
	sigslot::signal0<> search_cancelled;

protected:

	void search_in_reg();
	void search_in_mojang_dir();
	void search_in_client_dir();
	void search_in_path_env();
	void search_in_file_system();

	void emit_search_finished();
	void emit_search_cancelled();

private:
	std::thread thread_;
	volatile bool stop_search_ = true;
	bool is_searching_ = false;
};

