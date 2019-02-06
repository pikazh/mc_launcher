#ifndef __SHARE_MEMORY_H__
#define __SHARE_MEMORY_H__

#include <windows.h>
#include <string>

namespace IPC
{

class ShareMemory
{
public:
    ShareMemory();
    ~ShareMemory();
public:
    bool Create(const char* pName, UINT32 nSize);
    bool Connect(const char* pName);
    bool Disconnect();
    const char* GetName() const;
public:
    bool IsValid();
	UINT32 GetSize();
    unsigned char* GetData();
protected:
    HANDLE m_hFileMapping;
	UINT32 m_nSize;
    bool m_bValid;
    bool m_bOwner;
    std::string m_strName;
    unsigned char* m_pMem;
    unsigned char* m_pData;
};

}

#endif