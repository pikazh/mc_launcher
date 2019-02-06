#pragma once

#include <fstream>

class FileHelper
{
public:
	FileHelper();
	virtual ~FileHelper();

	bool equal_file_content(std::ifstream& in1, std::ifstream& in2);

	void copy_dir(const std::wstring &src, const std::wstring &dst);

	bool delete_dir(const std::wstring &dir);
};

