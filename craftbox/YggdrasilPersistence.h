#pragma once

#include "modes.h"
#include "aes.h"
#include "filters.h"

class YggdrasilPersistence
{
public:
	YggdrasilPersistence() = default;
	virtual ~YggdrasilPersistence() = default;

public:
	void load(std::string &access_token, std::string &client_token);
	void save(const std::string &access_token, const std::string &client_token);
protected:
	void init_aes_key_and_iv(unsigned char key[CryptoPP::AES::DEFAULT_KEYLENGTH], unsigned char iv[CryptoPP::AES::BLOCKSIZE]);
};