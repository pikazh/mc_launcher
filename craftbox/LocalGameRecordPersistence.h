#pragma once

#include <string>
#include <vector>

class LocalGameRecordPersistence
{
public:
	LocalGameRecordPersistence();
	virtual ~LocalGameRecordPersistence();

	void load();
	void save();
};

