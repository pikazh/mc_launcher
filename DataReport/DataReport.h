#pragma once
#include <stdint.h>

#define APP_STARTUP_EVENT							(1000)
#define APP_EXIT_EVENT								(APP_STARTUP_EVENT+1)
#define REGISTER_MC_PROTOCOL_FAILED_EVENT			(APP_EXIT_EVENT+1)
#define FETCH_UPDATE_INFO_FAILED_EVENT				(REGISTER_MC_PROTOCOL_FAILED_EVENT+1)
#define NEED_UPDATE_APP_EVENT						(FETCH_UPDATE_INFO_FAILED_EVENT+1)
#define FETCH_JAVA_INFO_FAILED_EVENT				(NEED_UPDATE_APP_EVENT+1)
#define DOWNLOAD_JAVA_FINISHED_EVENT				(FETCH_JAVA_INFO_FAILED_EVENT+1)
#define DOWNLOAD_UPDATE_PACK_FAILED_EVENT			(DOWNLOAD_JAVA_FINISHED_EVENT+1)
#define OPEN_CONFIG_WND_EVENT						(DOWNLOAD_UPDATE_PACK_FAILED_EVENT+1)
#define CLICK_CONFIG_WND_CONTROL					(OPEN_CONFIG_WND_EVENT+1)

class DataReport
{
public:
	static DataReport* global();

	DataReport();
	virtual ~DataReport();

public:
	void report_app_startup_event();
public:
	void report_event(uint32_t action, ...);
};

