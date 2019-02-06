#include "MinecraftDownloadRecordPersistence.h"
#include "base\PathService.h"
#include "base\JsonHelper.h"
#include "document.h"
#include "writer.h"
#include "minecraft_common\MinecraftInstanceManager.h"

#include <string>
#include <memory>
#include <fstream>
#include <codecvt>
#include <shlobj.h>

MinecraftDownloadRecordPersistence::MinecraftDownloadRecordPersistence()
{
}


MinecraftDownloadRecordPersistence::~MinecraftDownloadRecordPersistence()
{
}

void MinecraftDownloadRecordPersistence::load()
{
	std::wstring file = PathService().appdata_path();
	file += L"\\downloadrecord.dat";
	std::ifstream file_stream(file, std::ifstream::binary | std::ifstream::in);
	if (file_stream.is_open())
	{
		file_stream.seekg(0, std::ios_base::end);
		int sz = file_stream.tellg();
		file_stream.seekg(0, std::ios_base::beg);
		char* buf = new char[sz];
		std::unique_ptr<char[]> ptr(buf);
		file_stream.read(buf, sz);
		rapidjson::Document doc;
		doc.Parse(buf, sz);
		if (doc.IsArray())
		{
			auto array = doc.GetArray();
			for (auto it = array.begin(); it != array.end(); it++)
			{
				if (it->IsObject())
				{
					std::string base_dir;
					std::string version_str;
					std::string profile_url;
					if (JsonGetStringMemberValue(*it, "base_dir", base_dir) && !base_dir.empty()
						&& JsonGetStringMemberValue(*it, "version_str", version_str) && !version_str.empty()
						&& JsonGetStringMemberValue(*it, "profile_url", profile_url) && !profile_url.empty())
					{
						std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
						MinecraftInstanceManager::GetInstance()->download_mc_instance(converter.from_bytes(base_dir), converter.from_bytes(version_str), profile_url);
					}
				}
			}
		}
	}
	
}

void MinecraftDownloadRecordPersistence::save()
{
	std::wstring file = PathService().appdata_path();
	::SHCreateDirectory(nullptr, file.c_str());
	file += L"\\downloadrecord.dat";
	std::ofstream file_stream(file, std::ifstream::binary);
	if (file_stream.is_open())
	{
		auto downloading_insts = MinecraftInstanceManager::GetInstance()->downloading_instances();
		rapidjson::Document doc;
		rapidjson::Value root(rapidjson::kArrayType);
		for (auto it = downloading_insts.begin(); it != downloading_insts.end(); ++it)
		{
			for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2)
			{
				auto &obj = it2->second;
				std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
				rapidjson::Value v(rapidjson::kObjectType);
				v.AddMember("base_dir", rapidjson::Value(converter.to_bytes(obj->base_dir()).c_str(), doc.GetAllocator()), doc.GetAllocator());
				v.AddMember("version_str", rapidjson::Value(converter.to_bytes(obj->version_str()).c_str(), doc.GetAllocator()), doc.GetAllocator());
				v.AddMember("profile_url", rapidjson::Value(obj->profile_url().c_str(), doc.GetAllocator()), doc.GetAllocator());
				root.PushBack(v, doc.GetAllocator());
			}
			
		}

		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		if (root.Accept(writer))
		{
			file_stream.write(buffer.GetString(), buffer.GetSize());
		}
	}
}
