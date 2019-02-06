#include "MinecraftLauncher.h"
#include "minecraft_common/MinecraftCommon.h"
#include "minecraft_common/MinecraftProfile.h"
#include "glog/logging.h"
#include "minecraft_common/ZipCmdProxy.h"
#include "base/PathService.h"
#include "base/ResourceHelper.h"
#include "base/asynchronous_task_dispatch.h"
#include "minecraft_common/MinecraftUserIdentity.h"
#include "base/FileHelper.h"
#include "base/at_exit_manager.h"
#include "base/StringHelper.h"
#include "CraftBoxHelperStuff.h"
#include "resource.h"

#include <shlwapi.h>
#include <codecvt>
#include <algorithm>
#include <string>
#include <map>
#include <atlstr.h>
#include <atlconv.h>

MinecraftLauncher::MinecraftLauncher(std::shared_ptr<MinecraftProfile> inst, std::shared_ptr<MinecraftUserIdentity> user_id, const std::wstring &java_path, const std::wstring &java_param, const std::vector<std::pair<std::wstring, std::wstring>> extean_mc_params)
{
	mc_inst_ = inst;
	user_id_ = user_id;
	java_path_ = java_path;
	java_param_ = java_param;
	extra_minecraft_param_ = extean_mc_params;
	async_dispatch_in_create_thread_ = AsyncTaskDispatch::current();
}

MinecraftLauncher::MinecraftLauncher(std::shared_ptr<MinecraftProfile> inst, std::shared_ptr<MinecraftUserIdentity> user_id, const std::wstring & java_path, const std::wstring & java_param)
{
	mc_inst_ = inst;
	user_id_ = user_id;
	java_path_ = java_path;
	java_param_ = java_param;
	async_dispatch_in_create_thread_ = AsyncTaskDispatch::current();
}

MinecraftLauncher::~MinecraftLauncher()
{
	wait_thread();
}

bool MinecraftLauncher::launch()
{
	success_ = false;

	if (!mc_inst_ || !user_id_)
		return false;

	std::weak_ptr<MinecraftLauncher> weak_obj = this->shared_from_this();
	std::thread t([this, weak_obj]()
	{
		std::wstring cmd;
		cmd += L"\"";
		cmd += java_path_;
		cmd += L"\" ";
		cmd += java_param_;
		cmd += L" -Dfml.ignoreInvalidMinecraftCertificates=true -Dfml.ignorePatchDiscrepancies=true -Djava.library.path=\"";
		auto &client_dir = mc_inst_->client_dir();
		std::wstring native_lib_dir = client_dir + L"\\natives";
		extrace_native_libs(native_lib_dir);

		cmd += native_lib_dir;

		if (0 == user_id_->user_type().compare(L"craftbox"))
		{
			cmd += L";";
			static std::wstring extrace_craftbox_helper_dll_path = MinecraftLauncher::extract_craftboxhelper_dll();
			cmd += extrace_craftbox_helper_dll_path;
		}
		
		cmd += L"\" -cp ";

		std::wstring cp = L"\"";
		auto libs = mc_inst_->library_files();
		for (auto& lib : libs)
		{
			if (!PathFileExistsW(lib->file_path.c_str()))
			{
				std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
				LOG(ERROR) << "can not find " << converter.to_bytes(lib->file_path);
				emit_launch_processed_signal(weak_obj, false, kErrorFileLoss);
				return;
			}
			cp += lib->file_path;
			cp += L";";
		}

		std::wstring client_jar_path = mc_inst_->client_file().file_path;
		if (!PathFileExistsW(client_jar_path.c_str()))
		{
			client_jar_path = mc_inst_->client_dir() + L"\\" + mc_inst_->version_str() + L".jar";
			if (!::PathFileExistsW(client_jar_path.c_str()))
			{
				std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
				LOG(ERROR) << "can not find " << converter.to_bytes(client_jar_path);
				emit_launch_processed_signal(weak_obj, false, kErrorFileLoss);
				return;
			}
		}

		if (0 == user_id_->user_type().compare(L"craftbox"))
		{
			auto helper_replace = CraftBoxHelperStuff::global();
			if (!mc_inst_->has_forge())
			{
				std::wstring patched_client_path;
				if (helper_replace->replace(java_path_, client_jar_path, patched_client_path))
				{
					client_jar_path = patched_client_path;
				}
			}
			else
			{
				helper_replace->add_mod(java_path_, client_jar_path, mc_inst_->client_dir() + L"\\mods");
			}
		}
		
		cp += client_jar_path;
		
		cp += L"\" ";
		cmd += cp;
		cmd += std::wstring(mc_inst_->main_class().begin(), mc_inst_->main_class().end());

		std::wstring argument = std::wstring(mc_inst_->launch_argument().begin(), mc_inst_->launch_argument().end());
		replace_argument(argument);
		handle_external_argument(argument);
		cmd += L" ";
		cmd += argument;

		check_and_copy_files();

		STARTUPINFO startup_info = { 0 };
		startup_info.cb = sizeof(startup_info);

		PROCESS_INFORMATION pi = { 0 };

		if (::CreateProcessW(nullptr, (LPWSTR)cmd.c_str(), nullptr, nullptr, FALSE, 0, nullptr, client_dir.c_str(), &startup_info, &pi))
		{
			DWORD create_time = ::GetTickCount();
			CloseHandle(pi.hThread);
			::WaitForInputIdle(pi.hProcess, INFINITE);
			
			proc_info_.reset(new MinecraftProcessInfo);
			proc_info_->base_dir_ = mc_inst_->base_dir();
			proc_info_->version_dir_ = mc_inst_->version_str();
			proc_info_->proc_handle_ = pi.hProcess;
			proc_info_->proc_id_ = pi.dwProcessId;
			proc_info_->create_time = create_time;

			emit_launch_processed_signal(weak_obj, true, kNoError);

			return;
		}
		else
		{
			auto err = ::GetLastError();
			std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
			LOG(ERROR) << "create process with cmd " << converter.to_bytes(cmd) << L"failed with err: " << err;
			emit_launch_processed_signal(weak_obj, false, kErrorCannotCreateProcess);
			return;
		}
	});

	launcher_thread_ = std::move(t);
	return true;
}

bool MinecraftLauncher::extrace_native_libs(std::wstring extract_path)
{
	auto libs = mc_inst_->library_files();
	for (auto& lib : libs)
	{
		if((lib->flag & FILE_FLAG_NATIVE_LIBRARY) == 0)
			continue;

		ZipCmdProxy proxy;
		std::wstring cmd = L"x \"" + lib->file_path + L"\" -o\"" + extract_path + L"\" -y";
		int ret_code = proxy.run(cmd);
		if (ret_code != 0)
		{
			std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
			LOG(ERROR) << "run extrace " << converter.to_bytes(lib->file_path) << " to " << converter.to_bytes(extract_path) << "failed with err " << ret_code;
			return false;
		}
	}

	return true;
}

void MinecraftLauncher::replace_argument(std::wstring &argument)
{
	std::map<std::wstring, std::wstring> replace_string_map;
	replace_string_map[L"auth_player_name"] = user_id_->player_name();
	try
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
		replace_string_map[L"version_name"] = converter.from_bytes(mc_inst_->id());
	}
	catch (std::exception &e)
	{
		USES_CONVERSION;
		replace_string_map[L"version_name"] = ATL::CA2W(mc_inst_->id().c_str());
		LOG(ERROR) << "replace version_name with "<< mc_inst_->id() << "raise exception: " << e.what();
	}
	
	replace_string_map[L"game_directory"] = L"\"" + mc_inst_->client_dir() + L"\"";
	replace_string_map[L"assets_root"] = L"\"" + mc_inst_->assets_dir() + L"\"";
	replace_string_map[L"game_assets"] = L"\"" + mc_inst_->assets_dir() + L"\"";
	replace_string_map[L"assets_index_name"] = std::wstring(mc_inst_->assets().begin(), mc_inst_->assets().end());
	replace_string_map[L"auth_uuid"] = user_id_->player_uuid().empty()?L"\"\"":user_id_->player_uuid();
	replace_string_map[L"auth_access_token"] = user_id_->access_token().empty() ? L"\"\"" : user_id_->access_token();
	replace_string_map[L"user_type"] = user_id_->user_type().empty()? L"\"\"" : user_id_->user_type();
	replace_string_map[L"user_properties"] = user_id_->user_properties();
	replace_string_map[L"version_type"] = mc_inst_->version_type().empty() ? L"\"\"" : std::wstring(mc_inst_->version_type().begin(), mc_inst_->version_type().end());
	if (user_id_->access_token().empty())
	{
		replace_string_map[L"auth_session"] = L"\"\"";
	}
	else
	{
		replace_string_map[L"auth_session"] = L"token:" + user_id_->access_token() + L":" + user_id_->player_uuid();
	}

	size_t begin_index = 0;
	size_t end_index = 0;
	while (true)
	{
		begin_index = argument.find(L"${", end_index);
		if (begin_index == std::wstring::npos)
		{
			break;
		}
		
		std::wstring key;
		end_index = argument.find(L"}", begin_index+wcslen(L"${"));
		DCHECK(end_index != std::wstring::npos);
		if (end_index != std::wstring::npos)
		{
			key = argument.substr(begin_index + wcslen(L"${"), end_index - begin_index - wcslen(L"${"));
			std::transform(key.begin(), key.end(), key.begin(), towlower);
			auto it = replace_string_map.find(key);
			if (it == replace_string_map.end())
			{
				DCHECK(0);
			}
			else
			{
				argument.replace(begin_index, end_index - begin_index + 1, it->second);
				end_index = begin_index + it->second.length();
			}
		}
	}
	
}

void MinecraftLauncher::emit_launch_processed_signal(const std::weak_ptr<MinecraftLauncher> &weak_obj, bool success, LauncherErrorCode code)
{
	AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj, success, code]()
	{
		auto obj = weak_obj.lock();
		if (!!obj)
		{
			obj->success_ = success;
			obj->error_code_ = code;
			obj->launch_processed();
		}
		
	}), true);
}

void MinecraftLauncher::wait_thread()
{
	if (launcher_thread_.joinable())
		launcher_thread_.join();
}

void MinecraftLauncher::handle_external_argument(std::wstring &argument)
{
	for (auto &item : extra_minecraft_param_)
	{
		std::wstring key = L"--" + item.first+L" ";
		auto index = argument.find(key);
		if (index == std::wstring::npos)
		{
			argument.append(L" "+ key + item.second);
		}
		else
		{
			auto end = argument.find(L" --", index);
			if (end != std::wstring::npos)
			{
				argument = argument.replace(argument.begin() + index + key.length(), argument.begin()+end, item.second);
			}
			else
			{
				argument = argument.replace(argument.begin()+index + key.length(), argument.end(), item.second);
			}
		}
	}
}

void MinecraftLauncher::check_and_copy_files()
{
	WIN32_FIND_DATAW find_data;
	HANDLE find_obj = ::FindFirstFileW(std::wstring(mc_inst_->client_dir() + L"\\*.*").c_str(), &find_data);
	if (find_obj != INVALID_HANDLE_VALUE)
	{
		do 
		{
			if(wcscmp(find_data.cFileName, L".") == 0 
				|| wcscmp(find_data.cFileName, L"..") == 0)
				continue;

			if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				std::wstring dir = mc_inst_->client_dir() + L"\\";
				dir += find_data.cFileName;
				if (::PathIsDirectoryEmptyW(dir.c_str()))
				{
					::RemoveDirectoryW(dir.c_str());
				}
			}
		} while (::FindNextFileW(find_obj, &find_data));

		::FindClose(find_obj);
	}
	
	find_obj = ::FindFirstFileW(std::wstring(mc_inst_->base_dir() + L"\\*.*").c_str(), &find_data);
	if (find_obj != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (wcscmp(find_data.cFileName, L".") == 0
				|| wcscmp(find_data.cFileName, L"..") == 0)
				continue;

			if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (wcsicmp(find_data.cFileName, L"assets") == 0
					|| wcsicmp(find_data.cFileName, L"libraries") == 0
					|| wcsicmp(find_data.cFileName, L"logs") == 0
					|| wcsicmp(find_data.cFileName, L"crash-reports") == 0
					|| wcsicmp(find_data.cFileName, L"versions") == 0)
				{

				}
				else
				{
					std::wstring dst_dir = mc_inst_->client_dir() + L"\\" + find_data.cFileName;
					if(!::PathFileExistsW(dst_dir.c_str()))
						FileHelper().copy_dir(std::wstring(mc_inst_->base_dir() + L"\\" + find_data.cFileName), dst_dir);
				}
			}
			else
			{
				bool dont_copy = false;
				wchar_t *exclude_files[] = { L".log", L".png" };
				for (auto *exclude_file : exclude_files)
				{
					auto ptr = wcsstr(find_data.cFileName, exclude_file);
					if (ptr && *(ptr + wcslen(exclude_file)) == 0)
					{
						dont_copy = true;
						break;
					}
				}

				if (!dont_copy)
				{
					std::wstring dst_file = mc_inst_->client_dir() + L"\\" + find_data.cFileName;
					::CopyFileW(std::wstring(mc_inst_->base_dir() + L"\\" + find_data.cFileName).c_str(), dst_file.c_str(), TRUE);
				}
			}
		} while (::FindNextFileW(find_obj, &find_data));

		::FindClose(find_obj);
	}
}

std::wstring MinecraftLauncher::extract_craftboxhelper_dll()
{
	std::wstring path = PathService().appdata_path() + L"\\temp";
	std::wstring guid;
	guid_wstring(guid);
	path += L"\\";
	path += guid;
	::SHCreateDirectory(0, path.c_str());
	std::wstring temp_zip_file = path + L"\\temp.7z";
	if (extrace_resource(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDR_FILE_CB_HELPER_7Z), L"FILE", temp_zip_file))
	{
		ZipCmdProxy zip;
		std::wstring cmd = L"x \"" + temp_zip_file + L"\" -o\"" + path + L"\" -y";
		int ret = zip.run(cmd);
		DCHECK(ret == 0);

		::DeleteFileW(temp_zip_file.c_str());

		//Ëø×¡ÎÄ¼þ
		static HANDLE zip_file_lock1 = CreateFileW(std::wstring(path+L"\\CraftBoxHelper64.dll").c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
		static HANDLE zip_file_lock2 = CreateFileW(std::wstring(path + L"\\CraftBoxHelper32.dll").c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);

		return path;
	}
	else
	{
		DCHECK(0);
	}

	return L"";
}

