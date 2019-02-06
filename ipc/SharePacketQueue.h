#ifndef __SHARE_PACKAGE_QUEUE_H__
#define __SHARE_PACKAGE_QUEUE_H__

#include <windows.h>
#include <string>
#include "ipc.h"
#include "mutex.hpp"
#include "semaphore.hpp"
#include "ShareMemory.h"

namespace IPC
{

class SharePacketQueue
{
public:
    SharePacketQueue();
    ~SharePacketQueue();
public:
    bool Create(const char* pName);
    bool Connect(const char* pName);
    bool Disconnect();
public:
    const char* GetName();
	UINT32 GetPacketNum();
    void SetPacketNum(UINT32 newNum);
public:
    int Lock();
    int TryLock(DWORD ms);
    int Unlock();
    bool ActiveSend();
	UINT32 PushData(SharePacket* pData);
    bool PopData(SharePacket* pData);
    int WaitRecv(DWORD ms);
protected:
    void Clear();
	UINT32 WritePacket(BYTE* pWrite, SharePacket* pPacket);
	UINT32 ReadPacket(BYTE* pRead, UINT32 bufferLen, SharePacket* pPacket);
	UINT32 GetPacketDataNeedLen(SharePacket* pPacket);
	UINT32 GetMinPacketDataLen();
	UINT32 GetHeadPos();
    void SetHeadPos(UINT32 newPos);
	UINT32 GetTailPos();
    void SetTailPos(UINT32 newPos);
	UINT32 GetDivPos();
    void SetDivPos(UINT32 newPos);
	UINT32 GetBodyBufferSize();
    BYTE* GetBodyBuffer();
protected:
    System::Semaphore m_semSend;
    System::Mutex m_mutex;
    ShareMemory m_memDataQueue;
protected:
    bool m_bInit;
    bool m_bOwner;
    std::string m_strName;
};

}

#endif