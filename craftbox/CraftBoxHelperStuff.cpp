#include "CraftBoxHelperStuff.h"
#include "glog/logging.h"
#include "minecraft_common/MinecraftVersionDetecter.h"
#include "base/at_exit_manager.h"
#include "minecraft_common/ZipCmdProxy.h"
#include "base/StringHelper.h"
#include "base/ResourceHelper.h"
#include "base/PathService.h"
#include "resource.h"

#include <windows.h>
#include <codecvt>
#include <shlobj.h>

CraftBoxHelperStuff::CraftBoxHelperStuff()
{
}

CraftBoxHelperStuff::~CraftBoxHelperStuff()
{
	std::lock_guard<std::recursive_mutex> l(lock_);
	for(auto it = replaced_java_clients_.begin();
		it != replaced_java_clients_.end();
		++it)
	{
		for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2)
		{
			DeleteFileW(it2->second.c_str());
		}
	}
}

bool CraftBoxHelperStuff::replace(const std::wstring &java_path, const std::wstring &client_jar_path, std::wstring &result)
{
	if (!::PathFileExistsW(client_jar_path.c_str()))
		return false;

	std::string version;
	if (!check_version(java_path, client_jar_path, false, version))
		return false;

	if (found_replaced_jar_in_cache(version, client_jar_path, result))
		return true;

	static std::wstring mc_crack_zip_file = extrace_mccraft_zip();

	std::wstring extraced_class_path = PathService().appdata_path() + L"\\temp";	
	std::wstring guid;
	guid_wstring(guid);
	extraced_class_path += L"\\";
	extraced_class_path += guid;
	::SHCreateDirectory(0, extraced_class_path.c_str());
	
	std::wstring version_w(version.begin(), version.end());
	ZipCmdProxy zip;
	std::wstring cmd = L"x \"" + mc_crack_zip_file + L"\" -o\"" + extraced_class_path + L"\" " + version_w + L"\\* -y";
	int ret = zip.run(cmd);
	DCHECK(ret == 0);
	
	std::wstring temp_client_jar_path = PathService().appdata_path() + L"\\temp\\";
	guid_wstring(guid);
	temp_client_jar_path += guid;
	temp_client_jar_path += L".jar";
	if (!CopyFileW(client_jar_path.c_str(), temp_client_jar_path.c_str(), FALSE))
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
		LOG(ERROR) << "copy " << converter.to_bytes(client_jar_path)
			<< "to " << converter.to_bytes(temp_client_jar_path) << "failed with err:" << GetLastError();

		return false;
	}

	cmd = L"d \"" + temp_client_jar_path + L"\" META-INF";
	ret = zip.run(cmd);
	DCHECK(ret == 0);

	cmd = L"u \"" + temp_client_jar_path + L"\" \"" + extraced_class_path + L"\\" + version_w + L"\\*\"";
	ret = zip.run(cmd);
	DCHECK(ret == 0);

	set_replaced_jar_cache(version, client_jar_path, temp_client_jar_path);
	result = temp_client_jar_path;
	return true;
}

std::wstring CraftBoxHelperStuff::extrace_mccraft_zip()
{
	std::wstring path = PathService().appdata_path() + L"\\temp";
	std::wstring guid;
	guid_wstring(guid);
	path += L"\\";
	path += guid;
	::SHCreateDirectory(0, path.c_str());
	std::wstring temp_zip_file = path + L"\\crack.7z";
	if (extrace_resource(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDR_FILE_MCCRACK_7Z), L"FILE", temp_zip_file))
	{
		//锁住文件
		static HANDLE zip_file_lock = CreateFileW(temp_zip_file.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
		return temp_zip_file;

	}
	else
	{
		DCHECK(0);
		return L"";
	}
}

CraftBoxHelperStuff * CraftBoxHelperStuff::global()
{
	static std::shared_ptr<CraftBoxHelperStuff> global_obj = std::make_shared<CraftBoxHelperStuff>();
	return global_obj.get();
}

bool CraftBoxHelperStuff::found_replaced_jar_in_cache(const std::string &ver, const std::wstring &client_jar_path, std::wstring &replaced_jar_path)
{
	std::lock_guard<std::recursive_mutex> l(lock_);

	auto it = replaced_java_clients_.find(ver);
	if (it == replaced_java_clients_.end())
		return false;

	auto it2 = it->second.find(client_jar_path);
	if (it2 == it->second.end())
		return false;

	replaced_jar_path = it2->second;
	return true;
}

void CraftBoxHelperStuff::set_replaced_jar_cache(const std::string &ver, const std::wstring &client_jar_path, const std::wstring &replaced_jar_path)
{
	std::lock_guard<std::recursive_mutex> l(lock_);
	replaced_java_clients_[ver][client_jar_path] = replaced_jar_path;
}

bool CraftBoxHelperStuff::check_version(const std::wstring &java_path, const std::wstring &client_jar_path, bool mod_base, std::string &version)
{
	MinecraftVersionDetecter mc_ver_detecter;
	mc_ver_detecter.get_mc_version(java_path, client_jar_path, version);
	if (version.empty())
		return false;

	static char* support_versions[] =
	{
		"1.7.2",
		"1.7.10",
		"1.8",
		"1.8.1",
		"1.8.2",
		"1.8.3",
		"1.8.4",
		"1.8.5",
		"1.8.6",
		"1.8.7",
		"1.8.8",
		"1.8.9",
		"1.9",
		"1.9.1",
		"1.9.2",
		"1.9.3",
		"1.9.4"
	};

	static char* support_versions_mod_base[] =
	{
		"1.7.2",
		"1.7.10",
		"1.8",
		"1.8.8",
		"1.8.9",
		"1.9"
	};

	if (mod_base)
	{
		for (char* v : support_versions_mod_base)
		{
			if (version.compare(v) == 0)
			{
				return true;
			}
		}
	}
	else
	{
		for (char* v : support_versions)
		{
			if (version.compare(v) == 0)
			{
				return true;
			}
		}
	}

	return false;
}

void CraftBoxHelperStuff::add_mod(const std::wstring &java_path, const std::wstring &client_jar_path, const std::wstring &mod_dir)
{
	if (!::PathFileExistsW(client_jar_path.c_str()))
		return;

	std::string version;
	if (!check_version(java_path, client_jar_path, true, version))
		return;


	bool delete_old = true;
	WIN32_FIND_DATAW find_data;
	HANDLE find_obj = ::FindFirstFileW(std::wstring(mod_dir + L"\\*.*").c_str(), &find_data);
	if (find_obj != INVALID_HANDLE_VALUE)
	{
		do
		{
			if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				if (nullptr != StrStrIW(find_data.cFileName, L"CraftBoxHelper"))
				{
					std::wstring file_path = mod_dir + L"\\" + find_data.cFileName;
					delete_old &= (FALSE != ::DeleteFileW(file_path.c_str()));
				}
			}
		} while (::FindNextFileW(find_obj, &find_data));

		::FindClose(find_obj);
	}

	if (delete_old)
	{
		static std::wstring mc_crackmod_zip_file = extrace_mccraft_mod_zip();

		std::wstring extraced_class_path = PathService().appdata_path() + L"\\temp";
		std::wstring guid;
		guid_wstring(guid);
		extraced_class_path += L"\\";
		extraced_class_path += guid;
		::SHCreateDirectory(0, extraced_class_path.c_str());

		std::wstring version_w(version.begin(), version.end());
		ZipCmdProxy zip;
		std::wstring cmd = L"x \"" + mc_crackmod_zip_file + L"\" -o\"" + mod_dir + L"\" CraftBoxHelperMod" + version_w + L".jar -y";
		int ret = zip.run(cmd);
		DCHECK(ret == 0);
	}
	
}

std::wstring CraftBoxHelperStuff::extrace_mccraft_mod_zip()
{
	std::wstring path = PathService().appdata_path() + L"\\temp";
	std::wstring guid;
	guid_wstring(guid);
	path += L"\\";
	path += guid;
	::SHCreateDirectory(0, path.c_str());
	std::wstring temp_zip_file = path + L"\\crackmod.7z";
	if (extrace_resource(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDR_FILE_MCCRACKMOD_7Z), L"FILE", temp_zip_file))
	{
		//锁住文件
		static HANDLE zip_file_lock = CreateFileW(temp_zip_file.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
		return temp_zip_file;

	}
	else
	{
		DCHECK(0);
		return L"";
	}
}

