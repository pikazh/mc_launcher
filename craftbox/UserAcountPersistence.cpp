#include "UserAcountPersistence.h"
#include "base/system_info.h"
#include "base/PathService.h"
#include "document.h"
#include "base/JsonHelper.h"
#include "base/asynchronous_task_dispatch.h"

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include "md5.h"

#include <windows.h>
#include <fstream>
#include <shlobj.h>


UserAcountPersistence::UserAcountPersistence()
{
}


UserAcountPersistence::~UserAcountPersistence()
{
}

void UserAcountPersistence::load(std::string &data)
{
	auto yggdrasil_persistence_file = PathService().appdata_path() + L"\\user.dat";
	std::ifstream file(yggdrasil_persistence_file, std::ios_base::binary);
	if (file.is_open())
	{
		file.seekg(0, std::ios::end);
		uint32_t sz = file.tellg();
		if (sz == 0)
			return;

		file.seekg(0, std::ios::beg);
		char *buf = new char[sz];
		std::unique_ptr<char[]> auto_buf(buf);
		file.read(buf, sz);

		try
		{
			unsigned char key[CryptoPP::AES::DEFAULT_KEYLENGTH] = { 0 };
			unsigned char iv[CryptoPP::AES::BLOCKSIZE] = { 0 };
			init_aes_key_and_iv(key, iv);

			CryptoPP::AES::Decryption aesDecryption(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
			CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, iv);

			CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(data));
			stfDecryptor.Put(reinterpret_cast<const unsigned char*>(buf), sz);
			stfDecryptor.MessageEnd();
		}
		catch (...)
		{
			data.clear();
		}
	}
}

void UserAcountPersistence::save(const std::string &data)
{
	auto yggdrasil_persistence_file = PathService().appdata_path() + L"\\user.dat";
	::SHCreateDirectory(nullptr, PathService().appdata_path().c_str());
	std::ofstream save_file(yggdrasil_persistence_file, std::ios_base::binary);
	if (save_file.is_open())
	{
		unsigned char key[CryptoPP::AES::DEFAULT_KEYLENGTH] = { 0 };
		unsigned char iv[CryptoPP::AES::BLOCKSIZE] = { 0 };
		init_aes_key_and_iv(key, iv);

		std::string ciphertext;
		CryptoPP::AES::Encryption aesEncryption(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
		CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, iv);

		CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink(ciphertext));
		stfEncryptor.Put(reinterpret_cast<const unsigned char*>(data.c_str()), data.length());
		stfEncryptor.MessageEnd();

		save_file << ciphertext;
	}
}

void UserAcountPersistence::init_aes_key_and_iv(unsigned char key[CryptoPP::AES::DEFAULT_KEYLENGTH], unsigned char iv[CryptoPP::AES::BLOCKSIZE])
{
	char computer_name[MAX_COMPUTERNAME_LENGTH + 1] = { 0 };
	DWORD len = _countof(computer_name);
	GetComputerNameA(computer_name, &len);
	std::string raw_key = SystemInfo().cpu_model();
	raw_key += computer_name;

	byte mres[16] = { 0 };
	CryptoPP::Weak::MD5 md5;
	md5.Update((const byte*)raw_key.c_str(), raw_key.length());
	md5.Final(mres);

	memcpy(key, mres, CryptoPP::AES::DEFAULT_KEYLENGTH > 16 ? 16 : CryptoPP::AES::DEFAULT_KEYLENGTH);
	memcpy(iv, mres, CryptoPP::AES::BLOCKSIZE > 16 ? 16 : CryptoPP::AES::BLOCKSIZE);
}