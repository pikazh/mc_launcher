#pragma once
#include "IntergrationDownloadRecordPersistence.h"
#include "base\PathService.h"
#include "base\JsonHelper.h"
#include "document.h"
#include "writer.h"
#include "IntergrationVersionManager.h"

#include <string>
#include <memory>
#include <fstream>
#include <codecvt>
#include <shlobj.h>

IntergrationDownloadRecordPersistence::IntergrationDownloadRecordPersistence()
{
}

IntergrationDownloadRecordPersistence::~IntergrationDownloadRecordPersistence()
{
}

void IntergrationDownloadRecordPersistence::load()
{
	std::wstring file = PathService().appdata_path();
	file += L"\\integration_dl_record.dat";
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
			std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
			auto array = doc.GetArray();
			for (auto it = array.begin(); it != array.end(); it++)
			{
				if (it->IsObject())
				{
					std::string url;
					std::string download_path;
					std::string sha1;
					std::string extrace_path;
					std::string ver;
					uint64_t file_size;
					if (JsonGetStringMemberValue(*it, "url", url) && !url.empty()
						&& JsonGetStringMemberValue(*it, "download_path", download_path) && !download_path.empty()
						&& JsonGetStringMemberValue(*it, "sha1", sha1) && !sha1.empty()
						&& JsonGetUInt64MemberValue(*it, "size", file_size) && file_size != 0
						&& JsonGetStringMemberValue(*it, "extrace_path", extrace_path) && !extrace_path.empty()
						&& JsonGetStringMemberValue(*it, "ver", ver) && !ver.empty()
						)
					{
						IntergrationVersionManager::global()->download_instance(url, converter.from_bytes(download_path), sha1, file_size, converter.from_bytes(extrace_path), converter.from_bytes(ver));
					}
				}
			}
		}
	}
}

void IntergrationDownloadRecordPersistence::save()
{
	std::wstring file = PathService().appdata_path();
	::SHCreateDirectory(nullptr, file.c_str());
	file += L"\\integration_dl_record.dat";
	std::ofstream file_stream(file, std::ifstream::binary);
	if (file_stream.is_open())
	{
		auto downloading_insts = IntergrationVersionManager::global()->downloading_instances();
		rapidjson::Document doc;
		rapidjson::Value root(rapidjson::kArrayType);
		std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
		for (auto it = downloading_insts.begin(); it != downloading_insts.end(); ++it)
		{
			for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2)
			{
				auto &obj = std::get<0>(*it2);
				std::string sha1 = std::get<1>(*it2);
				uint64_t size = std::get<2>(*it2);
				std::string extrace_path = converter.to_bytes(std::get<3>(*it2));
				std::string ver = converter.to_bytes(std::get<4>(*it2));
				rapidjson::Value v(rapidjson::kObjectType);
				v.AddMember("url", rapidjson::Value(obj->url().c_str(), doc.GetAllocator()), doc.GetAllocator());
				v.AddMember("download_path", rapidjson::Value(converter.to_bytes(obj->download_file_path()).c_str(), doc.GetAllocator()), doc.GetAllocator());
				v.AddMember("sha1", rapidjson::Value(sha1.c_str(), doc.GetAllocator()), doc.GetAllocator());
				v.AddMember("size", size, doc.GetAllocator());
				v.AddMember("extrace_path", rapidjson::Value(extrace_path.c_str(), doc.GetAllocator()), doc.GetAllocator());
				v.AddMember("ver", rapidjson::Value(ver.c_str(), doc.GetAllocator()), doc.GetAllocator());
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
