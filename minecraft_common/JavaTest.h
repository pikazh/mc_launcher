#pragma once

#include <string>

class JavaTest
{
public:
	JavaTest();
	virtual ~JavaTest();

public:
	bool get_java_version(const std::wstring &java_path, std::string &ver, bool &is_x64);

protected:
	static std::wstring extract_test_jar();

private:
	std::wstring jar_path_;
};

