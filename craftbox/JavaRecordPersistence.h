#pragma once

#include <string>
#include <vector>

class JavaRecordPersistence
{
public:
	JavaRecordPersistence();
	virtual ~JavaRecordPersistence();

	void load();
	void save();
};

