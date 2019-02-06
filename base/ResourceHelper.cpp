#include "ResourceHelper.h"

bool extrace_resource(HMODULE module, LPCWSTR resource_name, LPCWSTR resource_type, const std::wstring & path)
{
	HRSRC hRsrc = FindResourceW(module, resource_name, resource_type);
	if (nullptr == hRsrc)
		return false;

	DWORD dwSize = SizeofResource(nullptr, hRsrc);
	assert(dwSize != 0);
	if (0 == dwSize)
		return false;

	HGLOBAL hGlobal = LoadResource(nullptr, hRsrc);
	if (NULL == hGlobal)
		return false;

	LPVOID pBuffer = LockResource(hGlobal);
	if (NULL == pBuffer)
		return false;

	std::wstring dir = path;
	*::PathFindFileNameW(dir.c_str()) = 0;
	::SHCreateDirectory(0, dir.c_str());

	FILE* fp = _wfopen(path.c_str(), L"wb");
	if (fp != nullptr)
	{
		fwrite(pBuffer, 1, dwSize, fp);
		fclose(fp);
		return true;
	}
	else
	{
		return false;
	}
}
