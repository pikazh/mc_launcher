#pragma once
#include <string>

class PathService
{
public:
	PathService() = default;
	virtual ~PathService() = default;

	std::wstring appdata_path();
	std::wstring temp_path();
	std::wstring exe_dir();
};

