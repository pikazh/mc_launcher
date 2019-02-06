#include "SharePacketQueue.h"
#include "memory_helper.h"

#define PACKETNUM_POS 0
#define HEAD_POS 1
#define TAIL_POS 2
#define DIV_POS 3

#define DEFAULT_DATAQUEUE_SIZE (1 * 1024 * 1024)	//1M

using namespace IPC;
using namespace System;
using namespace std;

SharePacketQueue::SharePacketQueue() :
    m_bInit(false),
    m_bOwner(false)
{
}

SharePacketQueue::~SharePacketQueue()
{
}

bool SharePacketQueue::Create(const char* pName)
{
    if (m_bInit)
        return false;
    m_strName = pName;
    string strName;
    strName = pName;
    strName += "_SPQ_MUTEX";
    if (!m_mutex.Create(strName.c_str()))
    {
        m_bInit = false;
        return false;
    }
    m_mutex.Lock();
    MutexGuard guard(m_mutex);
    strName = pName;
    strName += "_SPQ_SEMAPHORE";
    if (!m_semSend.Create(strName.c_str()))
    {
        Clear();
        m_bInit = false;
        return false;
    }

    strName = pName;
    strName += "_SPQ_SHAREMEMORY";
    if (!m_memDataQueue.Create(strName.c_str(), DEFAULT_DATAQUEUE_SIZE))
    {
        Clear();
        m_bInit = false;
        return false;
    }
    SetHeadPos(0);
    SetTailPos(0);
    SetPacketNum(0);
    SetDivPos(GetBodyBufferSize());
    m_bOwner = true;
    m_bInit = true;
    return true;
}

bool SharePacketQueue::Connect(const char* pName)
{
    if (m_bInit)
        return false;
    m_strName = pName;
    string strName;
    strName = pName;
    strName += "_SPQ_MUTEX";
    if (!m_mutex.Open(strName.c_str()))
    {
        m_bInit = false;
        return false;
    }
    m_mutex.Lock();
    MutexGuard guard(m_mutex);
    strName = pName;
    strName += "_SPQ_SEMAPHORE";
    if (!m_semSend.Open(strName.c_str()))
    {
        Clear();
        m_bInit = false;
        return false;
    }
    strName = pName;
    strName += "_SPQ_SHAREMEMORY";
    if (!m_memDataQueue.Connect(strName.c_str()))
    {
        Clear();
        m_bInit = false;
        return false;
    }
    m_bOwner = false;
    m_bInit = true;
    return true;
}

bool SharePacketQueue::Disconnect()
{
    if (!m_bInit)
        return false;
    Clear();
    m_bOwner = false;
    m_bInit = false;
    return true;
}

const char* SharePacketQueue::GetName()
{
    return m_strName.c_str();
}

UINT32 SharePacketQueue::GetPacketNum()
{
	UINT32* pRead = (UINT32*)m_memDataQueue.GetData();
    return pRead[PACKETNUM_POS];
}

void SharePacketQueue::SetPacketNum(UINT32 newNum)
{
	UINT32* pRead = (UINT32*)m_memDataQueue.GetData();
    pRead[PACKETNUM_POS] = newNum;
}

int SharePacketQueue::Lock()
{
    if (!m_bInit)
        return -1;
    return m_mutex.Lock();
}

int SharePacketQueue::TryLock(DWORD ms)
{
    if (!m_bInit)
        return -1;
    return m_mutex.TryLock(ms);
}

int SharePacketQueue::Unlock()
{
    if (!m_bInit)
        return -1;
    return m_mutex.Unlock();
}

bool SharePacketQueue::ActiveSend()
{
    return m_semSend.Active();
}

UINT32 SharePacketQueue::PushData(SharePacket* pData)
{
    if (!m_bInit)
        return 0;

	UINT32 result;
	UINT32 packetNeedLen = GetPacketDataNeedLen(pData);
    if (packetNeedLen == 0)
    { 
        return 0;
    }

	UINT32 bodyBufferSize = GetBodyBufferSize();
	UINT32 headPos = GetHeadPos();
	UINT32 tailPos = GetTailPos();
	UINT32 divPos = GetDivPos();

    if (headPos <= tailPos)
    {
        if (tailPos + packetNeedLen > bodyBufferSize)
        {
            if (packetNeedLen >= headPos)
            {
                return 0;
            }
            else
            {
                result = WritePacket(GetBodyBuffer(), pData);
                SetDivPos(tailPos);
                if (headPos == tailPos)
                {
                    SetHeadPos(0);
                }
                SetTailPos(packetNeedLen);
            }
        }
        else
        {
            result = WritePacket(GetBodyBuffer() + tailPos, pData);
            SetTailPos(tailPos + packetNeedLen);
            SetDivPos(bodyBufferSize);
        }
    }
    else
    {
        if (tailPos + packetNeedLen >= headPos)
        {
            return 0;
        }
        else
        {
            result = WritePacket(GetBodyBuffer() + tailPos, pData);
            SetTailPos(tailPos + packetNeedLen);
        }
    }

    SetPacketNum(GetPacketNum() + 1);
    ActiveSend();
    return result;
}

bool SharePacketQueue::PopData(SharePacket* pData)
{
    if (!m_bInit)
        return false;

	UINT32 num = GetPacketNum();
    if (num < 1)
        return false;

	UINT32 bodyBufferSize = GetBodyBufferSize();
	UINT32 headPos = GetHeadPos();
	UINT32 tailPos = GetTailPos();
	UINT32 divPos = GetDivPos();

    if (headPos == tailPos)
        return false;

    if (headPos >= divPos)
        return false;

	UINT32 readLen = ReadPacket(GetBodyBuffer() + headPos, divPos - headPos, pData);
    if (readLen <= 0)
        return false;

    if (headPos <= tailPos)
    {
        SetHeadPos(headPos + readLen);
    }
    else
    {
        if (headPos + readLen >= divPos)
        {
            SetHeadPos(0);
        }
        else
        {
            SetHeadPos(headPos + readLen);
        }
    }
    SetPacketNum(num - 1);
    return true;
}

void SharePacketQueue::Clear()
{
    m_mutex.Lock();
    m_memDataQueue.Disconnect();
    m_semSend.Release();
    m_mutex.Unlock();
    m_mutex.Release();
}

int SharePacketQueue::WaitRecv(DWORD ms)
{
    return m_semSend.TryLock(ms);
}

UINT32 SharePacketQueue::WritePacket(BYTE* pWrite, SharePacket* pPacket)
{
    BYTE* pStart = pWrite;
    int len = sizeof(SharePacketHeader);
    memcpy(pWrite, &pPacket->Header, len);
    pWrite += len;
    UINT32 bodyBufferLen = pPacket->Data.BodyBufferLen;
    if (bodyBufferLen > 0 && pPacket->Data.BodyBuffer)
    {
        *((UINT32*)pWrite) = bodyBufferLen;
        pWrite += sizeof(bodyBufferLen);
        memcpy(pWrite, pPacket->Data.BodyBuffer, bodyBufferLen);
        pWrite += bodyBufferLen;
    }
    else
    {
        *((UINT32*)pWrite) = 0;
        pWrite += sizeof(bodyBufferLen);
    }
    return (UINT32)(pWrite - pStart);
}

UINT32 SharePacketQueue::ReadPacket(BYTE* pRead, UINT32 bufferLen, SharePacket* pPacket)
{
    if (bufferLen < GetMinPacketDataLen()) return 0;
    BYTE* pStart = pRead;
    int len = sizeof(SharePacketHeader);
    memcpy(&pPacket->Header, pRead, len);
    pRead += len;
    UINT32 bodyBufferLen = *((UINT32*)pRead);
    pRead += sizeof(bodyBufferLen);
    if (bodyBufferLen > 0)
    {
        if (pPacket->Data.BodyBuffer)
        {
            MemoryPool->free(pPacket->Data.BodyBuffer);
        }
        pPacket->Data.BodyBufferLen = bodyBufferLen;
        pPacket->Data.BodyBuffer = (BYTE*)MemoryPool->alloca(bodyBufferLen);
        memcpy(pPacket->Data.BodyBuffer, pRead, bodyBufferLen);
        pRead += bodyBufferLen;
    }
    return (UINT32)(pRead - pStart);
}

UINT32 SharePacketQueue::GetPacketDataNeedLen(SharePacket* pPacket)
{
    return sizeof(SharePacketHeader) + sizeof(UINT32) + (UINT32)pPacket->Data.BodyBufferLen;
}

UINT32 SharePacketQueue::GetMinPacketDataLen()
{
    return sizeof(SharePacketHeader) + sizeof(UINT32);
}

UINT32 SharePacketQueue::GetHeadPos()
{
	UINT32* pRead = (UINT32*)m_memDataQueue.GetData();
    return pRead[HEAD_POS];
}

void SharePacketQueue::SetHeadPos(UINT32 newPos)
{
	UINT32* pRead = (UINT32*)m_memDataQueue.GetData();
    pRead[HEAD_POS] = newPos;
}

UINT32 SharePacketQueue::GetTailPos()
{
	UINT32* pRead = (UINT32*)m_memDataQueue.GetData();
    return pRead[TAIL_POS];
}

void SharePacketQueue::SetTailPos(UINT32 newPos)
{
	UINT32* pRead = (UINT32*)m_memDataQueue.GetData();
    pRead[TAIL_POS] = newPos; 
}

UINT32 SharePacketQueue::GetDivPos()
{
	UINT32* pRead = (UINT32*)m_memDataQueue.GetData();
    return pRead[DIV_POS];
}

void SharePacketQueue::SetDivPos(UINT32 newPos)
{
	UINT32* pRead = (UINT32*)m_memDataQueue.GetData();
    pRead[DIV_POS] = newPos;
}

UINT32 SharePacketQueue::GetBodyBufferSize()
{
    return m_memDataQueue.GetSize() - sizeof(UINT32) * 4;
}

BYTE* SharePacketQueue::GetBodyBuffer()
{
    return m_memDataQueue.GetData() + sizeof(UINT32) * 4;
}