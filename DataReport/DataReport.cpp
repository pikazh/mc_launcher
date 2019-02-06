#include "DataReport.h"

#include <memory>

DataReport* DataReport::global()
{
	static std::shared_ptr<DataReport> global_obj = std::make_shared<DataReport>();
	return global_obj.get();
}

DataReport::DataReport()
{
}

DataReport::~DataReport()
{
}

void DataReport::report_app_startup_event()
{
	
}

void DataReport::report_event(uint32_t action, ...)
{

}
