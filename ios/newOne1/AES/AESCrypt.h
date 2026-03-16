#pragma once

#include "rijndael-alg-fst.h"
#include "rijndael-api-fst.h"

#include <string>
#include <cstring>

class CAESCrypt // MODE_CBC or MODE_CFB128
{
public:
	CAESCrypt(int bitLen=256)
	{
		m_bitLen = bitLen;
		m_txtLen = bitLen/8;
		m_pEncryptState = 0;
		m_pDecryptState = 0;
		m_mode = MODE_CBC;
	}

	~CAESCrypt(void) {}

public:
	void SetCryptIV(char * iv, unsigned char mode = MODE_CBC)
	{
		//strcpy(iv,"@1B2c3D4e5F6g7H8");
		char iv1[32];
		int i, j, k;
		for (i = 0; i < 16; i++)
		{
			j = (unsigned char)iv[i];
			k = j / 16;
			iv1[i*2] = k > 9 ? 'a'+k-10 : '0'+k;
			k = j % 16;
			iv1[i*2+1] = k > 9 ? 'a'+k-10 : '0'+k;
		}
		m_mode = mode;
		int r = cipherInit(&m_EncryptCipherInst, mode, iv1);
		r = cipherInit(&m_DecryptCipherInst, mode, iv1);
	}

	void SetCryptKey(char * sz /*= "12345678901112131415202122232425"*/)
	{
		char key[64];
		int i, j, k;
		for (i = 0; i < m_txtLen; i++)
		{
			j = (unsigned char)sz[i];
			k = j / 16;
			key[i*2] = k > 9 ? 'a'+k-10 : '0'+k;
			k = j % 16;
			key[i*2+1] = k > 9 ? 'a'+k-10 : '0'+k;
		}
		int r = makeKey(&m_EncryptKeyInst, DIR_ENCRYPT, m_bitLen, key);
		r = makeKey(&m_DecryptKeyInst, DIR_DECRYPT, m_bitLen, key);
	}

	int Encrypt(unsigned char * plainBytes, int len, unsigned char * output)
	{
		if (m_mode == MODE_CFB128)
		{
			return blockEncrypt_CFB128(&m_EncryptCipherInst, &m_EncryptKeyInst, plainBytes, len, output, &m_pEncryptState);
		}

		int ret = 0;
		if (len%m_txtLen != 0)
		{
			unsigned char paddingValue = 16 - len%16;
			int new_len = len+paddingValue;
			unsigned char * new_bytes = new unsigned char [new_len];
			memcpy(new_bytes, plainBytes, len);
			for (int i = 0; i < paddingValue; i++) new_bytes[len+i] = paddingValue;

			ret = blockEncrypt(&m_EncryptCipherInst, &m_EncryptKeyInst, new_bytes, new_len*8, output) / 8;
			delete [] new_bytes;
		}
		else
		{
			ret = blockEncrypt(&m_EncryptCipherInst, &m_EncryptKeyInst, plainBytes, len*8, output) / 8;
		}
		//return padEncrypt(&m_EncryptCipherInst, &m_EncryptKeyInst, plaintext, len, output);
		return ret;
	}
  
  unsigned char * Encrypt(unsigned char * plainBytes, int len, int & outputLen)
  {
    if (m_mode == MODE_CFB128)
    {
      unsigned char * output = new unsigned char[len];
      outputLen = blockEncrypt_CFB128(&m_EncryptCipherInst, &m_EncryptKeyInst, plainBytes, len, output, &m_pEncryptState);
      return output;
    }

    unsigned char * output = nullptr;
    if (len%m_txtLen != 0)
    {
      unsigned char paddingValue = 16 - len%16;
      int new_len = len+paddingValue;
      unsigned char * new_bytes = new unsigned char [new_len];
      memcpy(new_bytes, plainBytes, len);
      for (int i = 0; i < paddingValue; i++) new_bytes[len+i] = paddingValue;

      output = new unsigned char[new_len];
      outputLen = blockEncrypt(&m_EncryptCipherInst, &m_EncryptKeyInst, new_bytes, new_len*8, output) / 8;
      delete [] new_bytes;
    }
    else
    {
      output = new unsigned char[len];
      outputLen = blockEncrypt(&m_EncryptCipherInst, &m_EncryptKeyInst, plainBytes, len*8, output) / 8;
    }
    //return padEncrypt(&m_EncryptCipherInst, &m_EncryptKeyInst, plaintext, len, output);
    return output;
  }

	int PadEncrypt(unsigned char * plainBytes, int len, unsigned char * output)
	{
		if (m_mode == MODE_CFB128)
		{
			return blockEncrypt_CFB128(&m_EncryptCipherInst, &m_EncryptKeyInst, plainBytes, len, output, &m_pEncryptState);
		}
		return padEncrypt(&m_EncryptCipherInst, &m_EncryptKeyInst, plainBytes, len*8, output) / 8;
	}

	int Decrypt(unsigned char * cipherBytes, int len, unsigned char * output, bool removePadding = true)
	{
		if (m_mode == MODE_CFB128)
		{
			return blockDecrypt_CFB128(&m_DecryptCipherInst, &m_DecryptKeyInst, cipherBytes, len, output, &m_pDecryptState);
		}
		int ret = blockDecrypt(&m_DecryptCipherInst, &m_DecryptKeyInst, cipherBytes, len*8, output);
		int paddingLen = 0;
		if (removePadding) 
		{
			unsigned char paddingValue = output[len-1];
			if (paddingValue > 0 && paddingValue <= 16)
			{
				paddingLen = paddingValue;
				for (int i = 0; i < paddingValue; i++) output[len-1-i] = 0;
			}
		}

		return m_mode == MODE_CFB128 ? ret : (ret/8 - paddingLen);
	}
  
  unsigned char * Decrypt(unsigned char * cipherBytes, int len, int & outputLen)
  {
    if (m_mode == MODE_CFB128)
    {
      unsigned char * output = new unsigned char[len];
      outputLen = blockDecrypt_CFB128(&m_DecryptCipherInst, &m_DecryptKeyInst, cipherBytes, len, output, &m_pDecryptState);
      return output;
    }
    unsigned char * output = new unsigned char[len];
    int ret = blockDecrypt(&m_DecryptCipherInst, &m_DecryptKeyInst, cipherBytes, len*8, output) / 8;
    return output;
  }

	int PadDecrypt(unsigned char * cipherBytes, int len, unsigned char * output)
	{
		if (m_mode == MODE_CFB128)
		{
			return blockDecrypt_CFB128(&m_DecryptCipherInst, &m_DecryptKeyInst, cipherBytes, len, output, &m_pDecryptState);
		}
		return padDecrypt(&m_DecryptCipherInst, &m_DecryptKeyInst, cipherBytes, len*8, output) / 8;
	}

	int DecryptHexStr_test(const char * ciphertext, int len, unsigned char * output, bool removePadding = true)
	{
		unsigned char * cipherBytes = new unsigned char[len];
		memset(cipherBytes, 0, len);
		parseHexStr2Byte(ciphertext, len, cipherBytes);

		int outputLen = len/2;
		int ret = m_mode == MODE_CFB128 ? blockDecrypt_CFB128(&m_DecryptCipherInst, &m_DecryptKeyInst, cipherBytes, outputLen, output, &m_pDecryptState) :
			blockDecrypt(&m_DecryptCipherInst, &m_DecryptKeyInst, cipherBytes, outputLen*8, output);

		int paddingLen = 0;
		if (removePadding)
		{
			unsigned char paddingValue = output[outputLen-1];
			if (paddingValue > 0 && paddingValue <= m_txtLen)
			{
				paddingLen = paddingValue;
				for (int i = 0; i < paddingValue; i++) output[outputLen-1-i] = 0;
			}
		}

		delete [] cipherBytes;

		return m_mode == MODE_CFB128 ? ret : (ret/8 - paddingLen);
	}

	int DecryptHexStr(const char * ciphertext, int len, unsigned char * output)
	{
		unsigned char * cipherBytes = new unsigned char[len];
		memset(cipherBytes, 0, len);
		parseHexStr2Byte(ciphertext, len, cipherBytes);

		int outputLen = len/2;
		int r = Decrypt(cipherBytes, outputLen, output);
		delete [] cipherBytes;
		return r;
	}
  
  unsigned char * DecryptHexStr(const char * ciphertext, int len, int & outputLen)
  {
    unsigned char * cipherBytes = new unsigned char[len];
    memset(cipherBytes, 0, len);
    parseHexStr2Byte(ciphertext, len, cipherBytes);

    int cipherLen = len/2;
    unsigned char * text = Decrypt(cipherBytes, cipherLen, outputLen);
    delete [] cipherBytes;
    return text;
  }

	std::string parseByte2HexStr(unsigned char * buf, int len)
	{
		std::string ret = "";
		for (int i = 0; i < len; i++)
		{
			char sz[3];
			sprintf(sz,"%02X", buf[i]);
			ret += sz;
		}
		return ret;
	}

	void parseHexStr2Byte(const char * _in, int _len, unsigned char * buf) 
	{
		int j = 0;
		for (int i = 0; i < _len; i+=2)
		{
			char h1 = _in[i];
			short v1 = 0;
			if (h1 >= 'A' && h1 <= 'F') v1 = h1-'A'+10;
			else if (h1 >= 'a' && h1 <= 'f') v1 = h1-'a'+10;
			else if (h1 >= '0' && h1 <= '9') v1 = h1-'0';
			char h2 = _in[i+1];
			short v2 = 0;
			if (h2 >= 'A' && h2 <= 'F') v2 = h2-'A'+10;
			else if (h2 >= 'a' && h2 <= 'f') v2 = h2-'a'+10;
			else if (h2 >= '0' && h2 <= '9') v2 = h2-'0';
			buf[j++] = v1*16+v2;
		}
	}

private:
	int m_bitLen;
	unsigned char m_mode;
	int m_txtLen;
	int m_pEncryptState;
	int m_pDecryptState;
	keyInstance m_EncryptKeyInst;
	keyInstance m_DecryptKeyInst;
	cipherInstance m_EncryptCipherInst;
	cipherInstance m_DecryptCipherInst;
};

