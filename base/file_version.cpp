#include "file_version.h"

#include <shlwapi.h>

bool FileVersion::get_file_version(const wchar_t* file_path_u16, LPDWORD main_ver, LPDWORD sub_ver, LPDWORD build_number, LPDWORD revision)
{
    if (file_path_u16 == nullptr || file_path_u16[0] == 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }

    if (!PathFileExistsW(file_path_u16))
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return false;
    }

    bool ret = false;
    DWORD dwLen = ::GetFileVersionInfoSizeW(file_path_u16, nullptr);
    if (dwLen != 0)
    {
        BYTE* buffer = new BYTE[dwLen];
        if (buffer)
        {
            if (::GetFileVersionInfoW(file_path_u16, 0, dwLen, buffer))
            {
                VS_FIXEDFILEINFO* lpFileInfo = nullptr;
                UINT nLen = 0;
                if (::VerQueryValueW(buffer, L"\\", (void**)&lpFileInfo, &nLen))
                {
                    if (main_ver)
                        *main_ver = (lpFileInfo->dwFileVersionMS >> 16);

                    if (sub_ver)
                        *sub_ver = (lpFileInfo->dwFileVersionMS & 0xFFFF);

                    if (build_number)
                        *build_number = (lpFileInfo->dwFileVersionLS >> 16);

                    if (revision)
                        *revision = (lpFileInfo->dwFileVersionLS & 0xFFFF);

                    ret = true;
                }
            }
            delete[] buffer;
        }
    }

    return ret;
}

std::string FileVersion::get_file_version(const wchar_t* file_path_u16)
{
    DWORD main_ver = 0;
    DWORD sub_ver = 0;
    DWORD build = 0;
    DWORD revision = 0;

    if (get_file_version(file_path_u16, &main_ver, &sub_ver, &build, &revision))
    {
        char buf[128] = { 0 };
        sprintf(buf, "%u.%u.%u.%u", main_ver, sub_ver, build, revision);
        return buf;
    }

    return std::string();
}


std::string FileVersion::app_file_version()
{
	return g_app_version;
}

std::string FileVersion::get_app_file_version()
{
	wchar_t path[1024] = { 0 };
	GetModuleFileNameW(nullptr, path, 1024);
	return FileVersion().get_file_version(path);
}

std::string FileVersion::g_app_version = get_app_file_version();
