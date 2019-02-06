#include "DownloadCacheManager.h"
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include "md5.h"
#include "PathService.h"
#include "FileHelper.h"
#include "sha.h"
#include "cryptlib.h"
#include "hex.h"
#include "files.h"
#include "glog/logging.h"

#include <shlwapi.h>
#include <assert.h>
#include <shlobj.h>

DownloadCacheManager* DownloadCacheManager::GetInstance()
{
	static std::shared_ptr<DownloadCacheManager> obj = std::make_shared<DownloadCacheManager>();
	return obj.get();
}

DownloadCacheManager::DownloadCacheManager()
{
}


DownloadCacheManager::~DownloadCacheManager()
{
}

std::wstring DownloadCacheManager::cache_entry_for_url(const std::string &url)
{
	std::string return_str;
	byte mres[16] = { 0 };
	CryptoPP::Weak::MD5 md5;
	md5.Update((const byte*)url.c_str(), url.length());
	md5.Final(mres);

	wchar_t md5_key[33] = { 0 };
	for (auto i = 0; i < 16; i++)
	{
		wsprintf(md5_key + i * 2, L"%02x", mres[i]);
	}

	return md5_key;
}

std::string DownloadCacheManager::get_if_modified_since(const std::wstring &key)
{
	std::lock_guard<decltype(lock_)> l(lock_);

	std::wstring cache_path = PathService().appdata_path() + L"\\cache\\" + key + L"\\";
	std::wstring key_file = cache_path + L"key";
	std::wstring data_file = cache_path + L"data";
	std::string return_str;
	if (!::PathFileExistsW(key_file.c_str()) || !::PathFileExistsW(data_file.c_str()))
	{
		return return_str;
	}

	if (::PathIsDirectoryW(key_file.c_str()))
	{
		FileHelper().delete_dir(key_file);
		return return_str;
	}

	if (::PathIsDirectoryW(data_file.c_str()))
	{
		FileHelper().delete_dir(data_file);
		return return_str;
	}

	std::string value;
	std::ifstream file_stream(data_file, std::ifstream::binary | std::ifstream::in);
	if (file_stream.is_open())
	{
		CryptoPP::SHA1 sha1;
		CryptoPP::FileSource fs(file_stream, true /* PumpAll */,
			new CryptoPP::HashFilter(sha1,
				new CryptoPP::HexEncoder(
					new CryptoPP::StringSink(value),
					false
				) // HexEncoder
			) // HashFilter
		); // FileSource

		file_stream.close();
	}
	else
	{
		return return_str;
	}

	if (value.empty())
		return return_str;

	char buf[1024] = { 0 };
	{
		std::ifstream file_stream(key_file, std::ifstream::binary | std::ifstream::in);
		file_stream.read(buf, 1024);
		for (auto i = 0; buf[i] != 0; i++)
		{
			buf[i] = ~buf[i];
		}
	}

	std::string buffer(buf);
	auto index = buffer.find(value);
	if (index == std::string::npos)
	{
		return return_str;
	}

	std::string if_modified_since = buffer.substr(index + value.length() + 1);
	return if_modified_since;
}

bool DownloadCacheManager::update_cache_file(const std::wstring &key, FILE *srcfp, const std::string &if_modify_since)
{
	std::lock_guard<decltype(lock_)> l(lock_);

	DCHECK(srcfp != nullptr);
	std::wstring cache_path = PathService().appdata_path() + L"\\cache\\" + key + L"\\";
	std::wstring key_file = cache_path + L"key";
	std::wstring data_file = cache_path + L"data";
	::SHCreateDirectory(0, cache_path.c_str());
	
	FILE *dst_fp = _wfopen(data_file.c_str(), L"wb");
	if (nullptr != dst_fp)
	{
		fflush(srcfp);
		fseek(srcfp, 0, SEEK_SET);

		size_t count = 0;
		char *buf = new char[1024 * 1024];
		do 
		{
			count = fread(buf, 1, 1024 * 1024, srcfp);
			if(count != 0)
				fwrite(buf, 1, count, dst_fp);
		} while (count == 1024*1024);
		
		delete[] buf;
		fclose(dst_fp);
	}
	else
	{
		return false;
	}

	std::string value;
	std::ifstream file_stream(data_file, std::ifstream::binary | std::ifstream::in);
	if (file_stream.is_open())
	{
		CryptoPP::SHA1 sha1;
		CryptoPP::FileSource fs(file_stream, true /* PumpAll */,
			new CryptoPP::HashFilter(sha1,
				new CryptoPP::HexEncoder(
					new CryptoPP::StringSink(value),
					false
				) // HexEncoder
			) // HashFilter
		); // FileSource

		file_stream.close();
	}
	else
	{
		return false;
	}

	FILE *key_fp = _wfopen(key_file.c_str(), L"wb");
	if (key_fp != nullptr)
	{
		std::string buf = value + " " + if_modify_since;
		auto len = buf.length();
		for (auto i = 0; i < len; i++)
		{
			buf[i] = ~buf[i];
		}

		fwrite(buf.c_str(), 1, len, key_fp);

		fclose(key_fp);

		return true;
	}
	else
	{
		return false;
	}
}

bool DownloadCacheManager::update_cache_file(const std::wstring &entry, const std::string &buffer, const std::string &if_modify_since)
{
	std::lock_guard<decltype(lock_)> l(lock_);

	DCHECK(!buffer.empty());
	std::wstring cache_path = PathService().appdata_path() + L"\\cache\\" + entry + L"\\";
	std::wstring key_file = cache_path + L"key";
	std::wstring data_file = cache_path + L"data";
	::SHCreateDirectory(0, cache_path.c_str());

	FILE *dst_fp = _wfopen(data_file.c_str(), L"wb");
	if (nullptr != dst_fp)
	{
		fwrite(buffer.c_str(), 1, buffer.length(), dst_fp);
		fclose(dst_fp);
	}
	else
	{
		return false;
	}

	std::string value;
	std::ifstream file_stream(data_file, std::ifstream::binary | std::ifstream::in);
	if (file_stream.is_open())
	{
		CryptoPP::SHA1 sha1;
		CryptoPP::FileSource fs(file_stream, true /* PumpAll */,
			new CryptoPP::HashFilter(sha1,
				new CryptoPP::HexEncoder(
					new CryptoPP::StringSink(value),
					false
				) // HexEncoder
			) // HashFilter
		); // FileSource

		file_stream.close();
	}
	else
	{
		return false;
	}

	FILE *key_fp = _wfopen(key_file.c_str(), L"wb");
	if (key_fp != nullptr)
	{
		std::string buf = value + " " + if_modify_since;
		auto len = buf.length();
		for (auto i = 0; i < len; i++)
		{
			buf[i] = ~buf[i];
		}

		fwrite(buf.c_str(), 1, len, key_fp);

		fclose(key_fp);

		return true;
	}
	else
	{
		return false;
	}
}

bool DownloadCacheManager::read_cache_file(const std::wstring &key, FILE *dstfp)
{
	std::lock_guard<decltype(lock_)> l(lock_);

	std::wstring cache_path = PathService().appdata_path() + L"\\cache\\" + key + L"\\";
	std::wstring data_file = cache_path + L"data";
	FILE *srcfp = _wfopen(data_file.c_str(), L"rb");
	if (srcfp != nullptr)
	{
		fflush(dstfp);
		fseek(dstfp, 0, SEEK_SET);

		size_t count = 0;
		char *buf = new char[1024 * 1024];
		do
		{
			count = fread(buf, 1, 1024 * 1024, srcfp);
			if (count != 0)
				fwrite(buf, 1, count, dstfp);
		} while (count == 1024 * 1024);

		delete[] buf;
		fclose(srcfp);

		return true;
	}
	else
	{
		return false;
	}
}

bool DownloadCacheManager::read_cache_file(const std::wstring &entry, std::string &buffer)
{
	std::lock_guard<decltype(lock_)> l(lock_);

	std::wstring cache_path = PathService().appdata_path() + L"\\cache\\" + entry + L"\\";
	std::wstring data_file = cache_path + L"data";
	FILE *srcfp = _wfopen(data_file.c_str(), L"rb");
	if (srcfp != nullptr)
	{
		size_t count = 0;
		char *buf = new char[1024 * 1024];
		do
		{
			count = fread(buf, 1, 1024 * 1024, srcfp);
			if (count != 0)
				buffer.append(buf, count);
		} while (count == 1024 * 1024);

		delete[] buf;
		fclose(srcfp);

		return true;
	}
	else
	{
		return false;
	}
}
