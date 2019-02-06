#pragma once

#include <string>
#include <vector>
#include <memory>
#include <stdint.h>

#include "base/HttpDownloadToFileWithCache.h"

class IntergrationDownloadRecordPersistence
{
public:
	IntergrationDownloadRecordPersistence();
	virtual ~IntergrationDownloadRecordPersistence();

	void load();
	void save();

};
