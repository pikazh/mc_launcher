#include "LocalGameRecordPersistence.h"
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

LocalGameRecordPersistence::LocalGameRecordPersistence()
{
}


LocalGameRecordPersistence::~LocalGameRecordPersistence()
{
}

void LocalGameRecordPersistence::load()
{
	std::wstring file = PathService().appdata_path();
	file += L"\\localgame.dat";
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
		if (doc.IsObject())
		{
			std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
			for (auto it = doc.MemberBegin(); it != doc.MemberEnd(); ++it)
			{
				if(!it->name.IsString() || it->name.GetStringLength() == 0)
					continue;

				std::string base_dir(it->name.GetString(), it->name.GetStringLength());
				std::wstring base_dir_wide = converter.from_bytes(base_dir);
				if(it->value.IsArray())
				{
					auto objs = it->value.GetArray();
					for (auto it = objs.begin(); it != objs.end(); ++it)
					{
						if(!it->IsObject())
							continue;

						auto ver_it = it->FindMember("ver");
						if(ver_it == it->MemberEnd())
							continue;
						
						if(!ver_it->value.IsString() || ver_it->value.GetStringLength() == 0)
							continue;
						std::wstring ver = converter.from_bytes(std::string(ver_it->value.GetString(), ver_it->value.GetStringLength()));

						std::wstring note;
						auto note_it = it->FindMember("note");
						if (note_it != it->MemberEnd() && note_it->value.IsString() && note_it->value.GetStringLength() != 0)
							note = converter.from_bytes(std::string(note_it->value.GetString(), note_it->value.GetStringLength()));

						MinecraftInstanceManager::GetInstance()->check_and_add_minecraft_instance(base_dir_wide, ver, note);

						auto default_it = it->FindMember("default");
						if (default_it != it->MemberEnd() && default_it->value.IsBool() && default_it->value.GetBool())
							MinecraftInstanceManager::GetInstance()->set_default_instance(base_dir_wide, ver);
					}
				}
			}
		}
	}
}

void LocalGameRecordPersistence::save()
{
	std::wstring file = PathService().appdata_path();
	::SHCreateDirectory(nullptr, file.c_str());
	file += L"\\localgame.dat";
	std::ofstream file_stream(file, std::ifstream::binary);
	if (file_stream.is_open())
	{
		rapidjson::Document doc;
		rapidjson::Value root(rapidjson::kObjectType);

		std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
		auto local_insts = MinecraftInstanceManager::GetInstance()->local_instances();
		auto default_inst = MinecraftInstanceManager::GetInstance()->default_local_inst().get();
		for (auto it = local_insts.begin(); it != local_insts.end(); ++it)
		{
			rapidjson::Value dirs_array(rapidjson::kArrayType);
			
			for (auto ver = it->second.begin(); ver != it->second.end(); ++ver)
			{
				rapidjson::Value inst_obj(rapidjson::kObjectType);
				inst_obj.AddMember("ver", rapidjson::Value(converter.to_bytes(ver->second->version_str).c_str(), doc.GetAllocator()), doc.GetAllocator());
				inst_obj.AddMember("note", rapidjson::Value(converter.to_bytes(ver->second->note).c_str(), doc.GetAllocator()), doc.GetAllocator());
				if (default_inst == ver->second.get())
				{
					inst_obj.AddMember("default", true, doc.GetAllocator());
				}
				dirs_array.PushBack(inst_obj, doc.GetAllocator());
			}
			root.AddMember(rapidjson::Value(converter.to_bytes(it->second.begin()->second->base_dir).c_str(), doc.GetAllocator()), dirs_array, doc.GetAllocator());
		}

		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		if (root.Accept(writer))
		{
			file_stream.write(buffer.GetString(), buffer.GetSize());
		}
	}
}
