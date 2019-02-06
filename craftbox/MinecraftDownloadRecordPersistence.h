#pragma once

#include <string>
#include <vector>

class MinecraftDownloadRecordPersistence
{
public:
	MinecraftDownloadRecordPersistence();
	virtual ~MinecraftDownloadRecordPersistence();

	void load();
	void save();
};

