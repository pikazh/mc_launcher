#include "AppConfig.h"
#include "base/PathService.h"
#include "base/pickle.h"

#include <shlobj.h>
#include <fstream>
#include <memory>
#include <assert.h>

AppConfig * AppConfig::global()
{
	static std::shared_ptr<AppConfig> global = std::make_shared<AppConfig>();
	return global.get();
}

AppConfig::AppConfig()
{
	load();
}

AppConfig::~AppConfig()
{
}

void AppConfig::load()
{
	__try
	{
		unsafe_load();
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{

	}
}

void AppConfig::save()
{
	auto profile_file = PathService().appdata_path();
	::SHCreateDirectory(nullptr, profile_file.c_str());
	profile_file += L"\\config.dat";
	std::ofstream file_stream(profile_file, std::ifstream::binary);
	if (file_stream.is_open())
	{
		Pickle pickle;
		pickle.write_integer(stores_.size());
		for (auto it = stores_.begin(); it != stores_.end(); ++it)
		{
			pickle.write_char_array((char*)it->first.c_str(), it->first.size()*sizeof(wchar_t));
			pickle.write_char_array((char*)it->second.c_str(), it->second.size()*sizeof(wchar_t));
		}

		size_t buf_size = 0, content_len = 0;
		auto ptr = pickle.get_raw_byte_array_ptr(buf_size, content_len);

		file_stream.write(ptr, buf_size);
	}

}

void AppConfig::set_value_for_key(const std::wstring & key, const std::wstring & value)
{
	stores_[key] = value;
}

std::wstring AppConfig::get_value_for_key(const std::wstring &key)
{
	auto it = stores_.find(key);
	if (it != stores_.end())
		return it->second;
	else
		return std::move(std::wstring());
}

void AppConfig::unsafe_load()
{
	auto profile_file = PathService().appdata_path();
	profile_file += L"\\config.dat";
	std::ifstream file_stream(profile_file, std::ios::in | std::ios::binary);
	if (file_stream.is_open())
	{
		auto file_size = file_stream.seekg(0, std::ios_base::end).tellg();
		file_stream.seekg(0, std::ios_base::beg);
		char *buf = new char[file_size];
		std::unique_ptr<char[]> ptr(buf);
		file_stream.read(buf, file_size);
		Pickle pickle;
		if (pickle.attach_to_byte_array(buf, file_size, [](void* ptr) {}))
		{
			PickleIterator it(pickle);
			std::wstring key;
			std::wstring value;
			uint32_t item_cnt = 0;

			item_cnt = it.read_pod<uint32_t>();
			for (auto i = 0; i < item_cnt; i++)
			{
				size_t sz = 0;
				auto ptr = it.read_char_array(sz);
				key.assign((wchar_t*)ptr, sz / sizeof(wchar_t));

				sz = 0;
				ptr = it.read_char_array(sz);
				value.assign((wchar_t*)ptr, sz / sizeof(wchar_t));

				stores_[key] = value;
			}
		}
		else
		{
			assert(0);
		}
	}
}
