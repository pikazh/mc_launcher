#include "FileHelper.h"

#include <algorithm>
#include <windows.h>
#include <shlobj.h>

FileHelper::FileHelper()
{
}


FileHelper::~FileHelper()
{
}

bool FileHelper::equal_file_content(std::ifstream& in1, std::ifstream& in2)
{
	std::ifstream::pos_type size1, size2;

	size1 = in1.seekg(0, std::ifstream::end).tellg();
	in1.seekg(0, std::ifstream::beg);

	size2 = in2.seekg(0, std::ifstream::end).tellg();
	in2.seekg(0, std::ifstream::beg);

	if (size1 != size2)
		return false;

	const size_t BLOCKSIZE = 4096;
	size_t remaining = size1;

	while (remaining)
	{
		char buffer1[BLOCKSIZE], buffer2[BLOCKSIZE];
		size_t size = std::min(BLOCKSIZE, remaining);

		in1.read(buffer1, size);
		in2.read(buffer2, size);

		if (0 != memcmp(buffer1, buffer2, size))
			return false;

		remaining -= size;
	}

	return true;
}

void FileHelper::copy_dir(const std::wstring &src, const std::wstring &dst)
{
	::SHCreateDirectory(nullptr, dst.c_str());

	WIN32_FIND_DATAW find_data;
	std::wstring src_dir = src + L"\\";
	std::wstring dst_dir = dst + L"\\";
	HANDLE find = ::FindFirstFileW(std::wstring(src_dir + L"*.*").c_str(), &find_data);
	if (find != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (wcscmp(find_data.cFileName, L"..") == 0 || wcscmp(find_data.cFileName, L".") == 0)
				continue;

			std::wstring src_file = src_dir + find_data.cFileName;
			std::wstring dst_file = dst_dir + find_data.cFileName;

			if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
			{
				copy_dir(src_file, dst_file);
			}
			else
			{
				::CopyFileW(src_file.c_str(), dst_file.c_str(), FALSE);
			}
		} while (::FindNextFileW(find, &find_data));

		::FindClose(find);
	}
}

bool FileHelper::delete_dir(const std::wstring &dir)
{
	WIN32_FIND_DATAW find_data;
	HANDLE find = ::FindFirstFileW(std::wstring(dir + L"\\*.*").c_str(), &find_data);
	bool ret = true;
	if (find != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (wcscmp(find_data.cFileName, L"..") == 0 || wcscmp(find_data.cFileName, L".") == 0)
				continue;

			if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
			{
				ret &= delete_dir(std::wstring(dir + L"\\" + find_data.cFileName));
			}
			else
			{
				ret &= (FALSE != ::DeleteFileW(std::wstring(dir + L"\\" + find_data.cFileName).c_str()));
			}
		} while (::FindNextFileW(find, &find_data));

		::FindClose(find);
	}

	ret &= (FALSE != ::RemoveDirectoryW(dir.c_str()));
	return ret;
}
