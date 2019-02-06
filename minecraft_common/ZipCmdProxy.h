#pragma once

#include <string>

class ZipCmdProxy
{
public:
	ZipCmdProxy();
	virtual ~ZipCmdProxy();

	int run(const std::wstring &param);
protected:
	static std::wstring uncompress_zip();

private:
	std::wstring g_uncompress_zip;

};

