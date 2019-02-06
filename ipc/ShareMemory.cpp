#include "ShareMemory.h"

using namespace IPC;
using namespace std;

ShareMemory::ShareMemory() :
    m_hFileMapping(NULL),
    m_nSize(0),
    m_bValid(false),
    m_bOwner(false),
    m_pData(NULL)
{

}

ShareMemory::~ShareMemory()
{
    Disconnect();
}

bool ShareMemory::Create(const char* pName, UINT32 nSize)
{
    if (m_bValid)
        return false;

    if (nSize > 0x7FFFFFFF)
    {
        m_bValid = false;
        return false;
    }

    m_hFileMapping = ::CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE | SEC_COMMIT, 0, nSize + sizeof(UINT32), pName);

    if (m_hFileMapping == NULL || m_hFileMapping == INVALID_HANDLE_VALUE)
    {
        m_bValid = false;
        return false;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
		_ASSERT(0);
        ::CloseHandle(m_hFileMapping);
        m_bValid = false;
        return false;
    }

    m_pMem = (unsigned char*)::MapViewOfFile(m_hFileMapping, FILE_MAP_WRITE | FILE_MAP_READ, 0, 0, 0);
    if (!m_pMem)
    {
        ::CloseHandle(m_hFileMapping);
        m_bValid = false;
        return false;
    }

	UINT32* pHeader = (UINT32*)m_pMem;
    pHeader[0] = nSize;
    m_pData = m_pMem + sizeof(UINT32);
    m_strName = pName;

    m_bValid = true;
    m_bOwner = true;
    return true;
}

bool ShareMemory::Connect(const char* pName)
{
    if (m_bValid)
        return false;

    m_hFileMapping = ::OpenFileMappingA(FILE_MAP_WRITE | FILE_MAP_READ, FALSE, pName);
    if (m_hFileMapping == NULL || m_hFileMapping == INVALID_HANDLE_VALUE)
    {
        m_bValid = false;
        return false;
    }

    m_pMem = (unsigned char*)::MapViewOfFile(m_hFileMapping, FILE_MAP_WRITE | FILE_MAP_READ, 0, 0, 0);
    if (m_pMem == NULL)
    {
        ::CloseHandle(m_hFileMapping);
        m_bValid = false;
        return false;
    }
    m_pData = m_pMem + sizeof(UINT32);
    m_strName = pName;
    m_bValid = true;
    m_bOwner = false;
    return true;
}

bool ShareMemory::Disconnect()
{
    if (!m_bValid)
        return false;

    if (m_hFileMapping == NULL || m_hFileMapping == INVALID_HANDLE_VALUE)
        return false;

    ::UnmapViewOfFile(m_pMem);
    ::CloseHandle(m_hFileMapping);
    m_pMem = NULL;
    m_pData = NULL;
    m_hFileMapping = NULL;
    m_bValid = false;
	m_strName.clear();
    return true;
}

bool ShareMemory::IsValid()
{
    return m_bValid;
}

const char* ShareMemory::GetName() const
{
    return m_strName.c_str();
}

UINT32 ShareMemory::GetSize()
{
    if (!m_bValid)
        return 0;
	UINT32* pHeader = (UINT32*)m_pMem;
    return pHeader[0];
}

BYTE* ShareMemory::GetData()
{
    if (!m_bValid)
        return NULL;
    return m_pData;
}