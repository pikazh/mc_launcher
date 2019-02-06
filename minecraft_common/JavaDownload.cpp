#include "JavaDownload.h"
#include "base/SystemHelper.h"
#include "MinecraftCommon.h"
#include "base/NetworkTaskDispatcher.h"
#include "base/PathService.h"
#include "base/at_exit_manager.h"
#include "base/system_info.h"
#include "glog/logging.h"
#include "base/JsonHelper.h"
#include "base/StringHelper.h"
#include "sha.h"
#include "cryptlib.h"
#include "hex.h"
#include "files.h"
#include "ZipCmdProxy.h"
#include "JavaTest.h"
#include "craftbox/JavaEnvironment.h"
#include "DataReport/DataReport.h"

#include <shlobj.h>

JavaDownload::JavaDownload()
{
	java_buffer_size_ = 2048;
	java_url_buffer_ = (char*)malloc(java_buffer_size_);
	memset(java_url_buffer_, 0, java_buffer_size_);

	this->state_changed.connect(this, &JavaDownload::on_network_task_state_changed);
}

JavaDownload::~JavaDownload()
{
	if (java_url_buffer_)
	{
		free(java_url_buffer_);
		java_url_buffer_ = nullptr;
	}
}

void JavaDownload::start_download(uint8_t java_version)
{
	set_cancel(false);
	download_success_ = false;
	java_download_items_.clear();
	buffer_content_len = 0;
	step_ = DL_URL;
	NetworkTaskDispatcher::global()->add_task(this->shared_from_this());

	std::string url = mcfe::g_java_info_url;
	if (SystemInfo().is_x64_system())
		url += "?sys=x64";
	else
		url += "?sys=x86";
	url += "&version=";
	url += std::to_string(java_version);
	easy().add<CURLOPT_URL>(url.c_str());

	easy().add<CURLOPT_WRITEFUNCTION>(JavaDownload::java_url_buffer_func);
	easy().add<CURLOPT_WRITEDATA>(this);
}

void JavaDownload::stop_download()
{
	set_cancel(true);
}

size_t JavaDownload::java_url_buffer_func(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	JavaDownload* pThis = (JavaDownload*)userdata;
	auto sz = size*nmemb;
	if (pThis)
	{
		while (pThis->buffer_content_len + sz > pThis->java_buffer_size_)
		{
			pThis->java_buffer_size_ <<= 1;
			pThis->java_url_buffer_ = (char*)realloc(pThis->java_url_buffer_, pThis->java_buffer_size_);
			memset(pThis->java_url_buffer_ + pThis->buffer_content_len, 0, pThis->java_buffer_size_ - pThis->buffer_content_len);
		}
		memcpy(pThis->java_url_buffer_ + pThis->buffer_content_len, ptr, sz);
		pThis->buffer_content_len += sz;
	}

	return sz;
}

size_t JavaDownload::save_java_zip_func(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	FILE* file_ptr = (FILE*)userdata;
	if (file_ptr)
	{
		return fwrite(ptr, size, nmemb, file_ptr);
	}
	return size*nmemb;
}

bool JavaDownload::on_before_schedule()
{
	if (step_ == DL_STEP::DL_CONTENT)
	{
		std::wstring dir = PathService().appdata_path() + L"\\temp";
		::SHCreateDirectory(0, dir.c_str());
		guid_wstring(java_zip_path_);
		java_zip_path_ = dir + L"\\" + java_zip_path_;
		file_ptr_ = _wfopen(java_zip_path_.c_str(), L"wb+");
		if (file_ptr_ == nullptr)
			return false;

		easy().add<CURLOPT_WRITEFUNCTION>(JavaDownload::save_java_zip_func);
		easy().add<CURLOPT_WRITEDATA>(file_ptr_);
	}

	__super::on_before_schedule();
	return true;
}

bool JavaDownload::on_after_schedule()
{
	__super::on_after_schedule();

	if (step_ == DL_URL)
	{
		if (is_cancelled())
		{
			DataReport::global()->report_event(FETCH_JAVA_INFO_FAILED_EVENT, 0);
			return true;
		}
		else if (curl_result() != CURLE_OK)
		{
			DataReport::global()->report_event(FETCH_JAVA_INFO_FAILED_EVENT, 1);
			LOG(ERROR) << "download java url list failed with curl err:" << curl_result();
			return true; 
		}

		document_.Parse(java_url_buffer_, buffer_content_len);
		if (document_.IsArray())
		{
			auto array = document_.GetArray();
			for (auto obj = array.Begin(); obj != array.end(); obj++)
			{
				std::shared_ptr<JavaDLItem> item = std::make_shared<JavaDLItem>();
				if (!obj->IsObject())
				{
					continue;
				}
				if (!JsonGetStringMemberValue(*obj, "url", item->url))
				{
					continue;
				}
				if (!JsonGetStringMemberValue(*obj, "sha1", item->sha1))
				{
					continue;
				}

				java_download_items_.push_back(item);
			}
		}
		else
		{
			DataReport::global()->report_event(FETCH_JAVA_INFO_FAILED_EVENT, 2);
		}
	}
	else if (step_ == DL_CONTENT)
	{
		auto info = easy().get_info<char>(CURLINFO_EFFECTIVE_URL);
		auto &url = info.get().second;

		download_success_ = false;
		if (is_cancelled())
		{
			DataReport::global()->report_event(DOWNLOAD_JAVA_FINISHED_EVENT, false, 0);
			return true;
		}
		else if (curl_result() != CURLE_OK)
		{
			DataReport::global()->report_event(DOWNLOAD_JAVA_FINISHED_EVENT, false, 1);
			LOG(ERROR) << "download" << url << "failed with curl err:" << curl_result();
			return true;
		}

		long http_rescode = 0;
		curl_easy_getinfo(easy().get_curl(), CURLINFO_RESPONSE_CODE, &http_rescode);
		if (http_rescode != 200)
		{
			DataReport::global()->report_event(DOWNLOAD_JAVA_FINISHED_EVENT, false, 2, http_rescode);
			LOG(ERROR) << "download" << url << "failed with http code:" << http_rescode;
		}

		DCHECK(file_ptr_);
		if (file_ptr_)
		{
			fclose(file_ptr_);
			file_ptr_ = nullptr;
		}

		download_success_ = true;
	}

	return true;
}

void JavaDownload::on_network_task_state_changed(std::shared_ptr<NetworkTask>, State state)
{
	if (state == kStop)
	{
		if (is_cancelled())
		{
			on_download_java_failed();
			return;
		}

		if (step_ == DL_STEP::DL_URL)
		{
			download_java_zip();
		}
		else if (step_ == DL_STEP::DL_CONTENT)
		{
			if (download_success_)
			{
				if (verify_download_file())
				{
					std::weak_ptr<JavaDownload> weak_obj = std::dynamic_pointer_cast<JavaDownload>(this->shared_from_this());
					std::thread t([weak_obj]()
					{
						auto obj = weak_obj.lock();
						if (!!obj)
						{
							obj->step_ = DL_STEP::DL_UNCOMPRESS;
							std::wstring cmd = L"x \"" + obj->java_zip_path_ + L"\" -o\"" + obj->download_dir() + L"\" -y";

							ZipCmdProxy proxy;
							proxy.run(cmd);

							{
								bool is_x64_system_ = SystemInfo().is_x64_system();
								std::list<std::wstring> search_dirs;
								search_dirs.push_back(obj->download_dir());

								while (!search_dirs.empty())
								{
									std::wstring path = search_dirs.front();
									search_dirs.pop_front();
									WIN32_FIND_DATAW findFileData;
									HANDLE hFind = FindFirstFileW((path + L"\\*.*").c_str(), &findFileData);
									if (hFind != INVALID_HANDLE_VALUE)
									{
										AtExitManager at_exit;
										at_exit.register_callback(make_closure(::FindClose, hFind));

										do
										{
											if (wcscmp(findFileData.cFileName, L".") == 0 || wcscmp(findFileData.cFileName, L"..") == 0)
												continue;

											if ((findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
											{
												search_dirs.push_back(path + L"\\" + findFileData.cFileName);
												continue;
											}
											else if ((findFileData.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_SPARSE_FILE)) != 0)
											{
												continue;
											}

											if (wcsicmp(findFileData.cFileName, L"Javaw.exe") == 0)
											{
												std::wstring file_path = path + L"\\" + findFileData.cFileName;
												JavaEnvironment::GetInstance()->try_add_java(file_path, true);
											}
										} while (::FindNextFileW(hFind, &findFileData));
									}
								}
							}

							DataReport::global()->report_event(DOWNLOAD_JAVA_FINISHED_EVENT, true);
							obj->on_download_java_success();
						}
					});
					t.detach();
				}
				else
				{
					DataReport::global()->report_event(DOWNLOAD_JAVA_FINISHED_EVENT, false, 4);
				}
			}
			else
			{
				//try next item
				download_java_zip();
			}
		}
	}
}

void JavaDownload::download_java_zip()
{
	if (java_download_items_.empty())
	{
		on_download_java_failed();
	}
	else
	{
		std::weak_ptr<JavaDownload> weak_obj = std::dynamic_pointer_cast<JavaDownload>(this->shared_from_this());
		AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj]()
		{
			auto obj = weak_obj.lock();
			if (!!obj)
			{
				obj->step_ = DL_STEP::DL_CONTENT;

				obj->current_dl_item_ = obj->java_download_items_.front();
				obj->java_download_items_.pop_front();

				obj->easy().add<CURLOPT_URL>(obj->current_dl_item_->url.c_str());
				NetworkTaskDispatcher::global()->add_task(obj);
			}
			
		}), true);
		
	}
}

void JavaDownload::on_download_java_failed()
{
	std::weak_ptr<JavaDownload> weak_obj = std::dynamic_pointer_cast<JavaDownload>(this->shared_from_this());
	AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj]()
	{
		auto obj = weak_obj.lock();
		if (!!obj)
		{
			obj->step_ = DL_NOT_START;
			obj->download_java_failed();
		}

	}), true);
}

void JavaDownload::on_download_java_success()
{
	std::weak_ptr<JavaDownload> weak_obj = std::dynamic_pointer_cast<JavaDownload>(this->shared_from_this());
	AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj]()
	{
		auto obj = weak_obj.lock();
		if (!!obj)
		{
			obj->step_ = DL_NOT_START;
			obj->download_java_success();
		}

	}), true);
}

bool JavaDownload::verify_download_file()
{
	std::ifstream file_stream(java_zip_path_, std::ifstream::binary | std::ifstream::in);
	if (!file_stream.is_open())
	{
		DCHECK(0);
		return false;
	}
	std::string value;
	{
		CryptoPP::SHA1 hash;
		CryptoPP::FileSource fs(file_stream, true /* PumpAll */,
			new CryptoPP::HashFilter(hash,
				new CryptoPP::HexEncoder(
					new CryptoPP::StringSink(value),
					false
					) // HexEncoder
				) // HashFilter
			); // FileSource
	}

	std::transform(current_dl_item_->sha1.begin(), current_dl_item_->sha1.end(), current_dl_item_->sha1.begin(), ::tolower);
	if (0 == current_dl_item_->sha1.compare(value))
	{
		return true;
	}

	return false;
}
