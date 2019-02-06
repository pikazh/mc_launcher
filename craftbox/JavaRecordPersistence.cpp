#include "JavaRecordPersistence.h"
#include "base/PathService.h"
#include "base/JsonHelper.h"
#include "document.h"
#include "writer.h"
#include "JavaEnvironment.h"

#include <string>
#include <memory>
#include <fstream>
#include <codecvt>
#include <shlobj.h>

JavaRecordPersistence::JavaRecordPersistence()
{
}


JavaRecordPersistence::~JavaRecordPersistence()
{
}

void JavaRecordPersistence::load()
{
	std::wstring file = PathService().appdata_path();
	file += L"\\java.dat";
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
			for (auto it = doc.Begin(); it != doc.End(); ++it)
			{
				if(!it->IsObject())
					continue;

				std::string path;
				JsonGetStringMemberValue(*it, "path", path);
				if(path.empty())
					continue;

				std::wstring java_path_wide = converter.from_bytes(path);
				if (JavaEnvironment::GetInstance()->try_add_java(java_path_wide))
				{
					bool default_java = false;
					JsonGetBoolMemberValue(*it, "default", default_java);
					if (default_java)
					{
						JavaEnvironment::GetInstance()->set_select_java_path(java_path_wide);
					}
				}

			}
		}
	}
}

void JavaRecordPersistence::save()
{
	std::wstring file = PathService().appdata_path();
	::SHCreateDirectory(nullptr, file.c_str());
	file += L"\\java.dat";
	std::ofstream file_stream(file, std::ifstream::binary);
	if (file_stream.is_open())
	{
		rapidjson::Document doc;
		rapidjson::Value root(rapidjson::kArrayType);

		std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
		auto javas = JavaEnvironment::GetInstance()->java_paths_and_vers();
		auto default_java = JavaEnvironment::GetInstance()->selected_java_path();
		for (auto it = javas.begin(); it != javas.end(); ++it)
		{
			std::string path = converter.to_bytes(it->first);
			rapidjson::Value java_obj(rapidjson::kObjectType);
			java_obj.AddMember("path", rapidjson::Value(path.c_str(), path.length(), doc.GetAllocator()), doc.GetAllocator());
			if (it->first.compare(default_java) == 0)
			{
				java_obj.AddMember("default", true, doc.GetAllocator());
			}
			root.PushBack(java_obj, doc.GetAllocator());
		}

		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		if (root.Accept(writer))
		{
			file_stream.write(buffer.GetString(), buffer.GetSize());
		}
	}
}
