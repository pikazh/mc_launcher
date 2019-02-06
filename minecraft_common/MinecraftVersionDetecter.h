#pragma once

#include <string>

class MinecraftVersionDetecter
{
public:
	MinecraftVersionDetecter();
	virtual ~MinecraftVersionDetecter();

	bool get_mc_version(const std::wstring &java_path, const std::wstring &client_jar_path, std::string &ver);

protected:
	static std::wstring extract_detecter_jar();
	static std::wstring extract_asm_jar();
private:
	std::wstring detecter_jar_path_;
	std::wstring asm_jar_path_;
};

