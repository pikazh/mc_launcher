#pragma once

#include "modes.h"
#include "aes.h"
#include "filters.h"

class UserAcountPersistence
{
public:
	UserAcountPersistence();
	virtual ~UserAcountPersistence();

	void load(std::string &data);
	void save(const std::string &data);

protected:
	void init_aes_key_and_iv(unsigned char key[CryptoPP::AES::DEFAULT_KEYLENGTH], unsigned char iv[CryptoPP::AES::BLOCKSIZE]);
};
