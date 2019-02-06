#include <windows.h>

#include "ipc.h"
#include "Channel.h"
#include "ChannelManager.h"
#include "ShareMemory.h"
#include "memory_helper.h"

using namespace IPC;

inline HSHAREMEM ObjectToHandle(ShareMemory* pObject)
{
    HSHAREMEM hObj;
    hObj.pvoid = pObject;
    return hObj;
}

inline ShareMemory* HandleToObject(HSHAREMEM hMem)
{
    return (ShareMemory*)hMem.pvoid;
}

inline HCHANNEL ObjectToHandle(Channel* pObject)
{
    HCHANNEL hObj;
    hObj.pvoid = pObject;
    return hObj;
}

inline Channel* HandleToObject(HCHANNEL hMem)
{
    return (Channel*)hMem.pvoid;
}

bool IPCStartup(const char* pszName)
{
    return ChannelManager::Instance().Start(pszName);
}

bool IPCCleanup()
{
    if (ChannelManager::Instance().Stop())
    {
        ChannelManager::Instance().WaitForEnd();
        return true;
    }
    return false;
}

bool IPCTryCleanup()
{
    if (ChannelManager::Instance().TryStop())
    {
        ChannelManager::Instance().WaitForEnd();
        return true;
    }
    return false;
}

bool IPCIsActive()
{
    return ChannelManager::Instance().IsActive();
}

const char* IPCGetName()
{
    return ChannelManager::Instance().GetName();
}

bool IPCCheckActive(const char* pszName)
{
    return ChannelManager::Instance().CheckActive(pszName);
}

HCHANNEL ChannelCreate(UINT16 nPort)
{
    HCHANNEL hChannel;
    hChannel.handle = 0;
    Channel* pChannel = Channel::Create(nPort);
    if (pChannel)
    {
        hChannel = ObjectToHandle(pChannel);
        if (!hChannel.handle)
        {
            pChannel->Destroy();
			pChannel = NULL;
        }
    }
    return hChannel;
}

bool ChannelDestroy(HCHANNEL hChannel)
{
    Channel* pChannel = HandleToObject(hChannel);
    if (pChannel)
    {
        return pChannel->Destroy();
    }
    return false;
}

bool ChannelClose(HCHANNEL hChannel)
{
    Channel* pChannel = HandleToObject(hChannel);
    if (pChannel)
    {
        return pChannel->Close();
    }
    return false;
}

int ChannelSend(HCHANNEL hChannel, const char* pszName, UINT16 uiProtocolID, unsigned char* pBuffer, UINT32 len)
{
    Channel* pChannel = HandleToObject(hChannel);
    if (pChannel)
    {
        return pChannel->Send(pszName, uiProtocolID, pBuffer, len);
    }
    return 0;
}

int ChannelSendRequest(HCHANNEL hChannel, const char* pszName, UINT16 uiProtocolID, unsigned char* pBuffer, UINT32 len)
{
    Channel* pChannel = HandleToObject(hChannel);
    if (pChannel)
    {
        return pChannel->SendRequest(pszName, uiProtocolID, pBuffer, len);
    }
    return 0;
}

int ChannelSendRequestSync(HCHANNEL hChannel, const char* pszName, UINT16 uiProtocolID, unsigned char* pBuffer, UINT32 len, PSharePacket* ppResponsePacket, UINT nTimeout)
{
    Channel* pChannel = HandleToObject(hChannel);
    if (pChannel && ppResponsePacket)
    {
        return pChannel->SendRequestSync(pszName, uiProtocolID, pBuffer, len, *ppResponsePacket, nTimeout);
    }
    return 0;
}

int ChannelSendResponse(HCHANNEL hChannel, const char* pszName, UINT16 uiProtocolID, unsigned char* pBuffer, UINT32 len, UINT32 nSeq)
{
    Channel* pChannel = HandleToObject(hChannel);
    if (pChannel)
    {
        return pChannel->SendResponse(pszName, uiProtocolID, pBuffer, len, nSeq);
    }
    return 0;
}

int ChannelWaitForEvent(HCHANNEL hChannel, int nTimeout)
{
    Channel* pChannel = HandleToObject(hChannel);
    if (pChannel)
    {
        return pChannel->WaitForEvent(nTimeout);
    }
    return CHANNEL_EVENT_ERROR;
}

PSharePacket ChannelGetRecvPacket(HCHANNEL hChannel)
{
    Channel* pChannel = HandleToObject(hChannel);
    if (pChannel)
    {
        return pChannel->GetRecvPacket();
    }
    return NULL;
}

bool ChannelGetDisconnect(HCHANNEL hChannel, PChannelAddr pChannelAddr)
{
    Channel* pChannel = HandleToObject(hChannel);
    if (pChannel)
    {
        return pChannel->GetDisconnect(pChannelAddr);
    }
    return false;
}

PSharePacket CreateSharePacket()
{
    PSharePacket pSharePacket = SharePacketPool->popObject();
    memset(pSharePacket, 0, sizeof(SharePacket));
    return pSharePacket;
}

bool DestroySharePacket(PSharePacket pPacket)
{
    if (pPacket)
    {
        if (pPacket->Data.BodyBuffer)
        {
            MemoryPool->free(pPacket->Data.BodyBuffer);
        }
        SharePacketPool->pushObject(pPacket);
        return true;
    }
    return false;
}

HSHAREMEM ShareMemoryCreate(const char* pszName, UINT32 nSize)
{
    HSHAREMEM hMem;
    hMem.handle = 0;
    ShareMemory* pShareMem = new ShareMemory();
    if (pShareMem->Create(pszName, nSize))
    {
        hMem = ObjectToHandle(pShareMem);
    }
    else
    {
        delete pShareMem;
        return hMem;
    }

    if (!hMem.handle)
    {
        delete pShareMem;
    }
    return hMem;
}

HSHAREMEM ShareMemoryOpen(const char* pszName)
{
    HSHAREMEM hMem;
    hMem.handle = 0;
    ShareMemory* pShareMem = new ShareMemory();
    if (pShareMem->Connect(pszName))
    {
        hMem = ObjectToHandle(pShareMem);
    }
    else
    {
        delete pShareMem;
        return hMem;
    }

    if (!hMem.handle)
    {
        delete pShareMem;
    }
    return hMem;
}

bool ShareMemoryDestroy(HSHAREMEM hShareMem)
{
    ShareMemory* pShareMem = HandleToObject(hShareMem);
    if (pShareMem)
    {
        delete pShareMem;
        return true;
    }
    else
    {
        return false;
    }
}

bool ShareMemoryConnect(HSHAREMEM hShareMem, const char* pszName)
{
    ShareMemory* pShareMem = HandleToObject(hShareMem);
    if (pShareMem)
    {
        return pShareMem->Connect(pszName);
    }
    return false;
}

bool ShareMemoryDisconnect(HSHAREMEM hShareMem)
{
    ShareMemory* pShareMem = HandleToObject(hShareMem);
    if (pShareMem)
    {
        return pShareMem->Disconnect();
    }
    return false;
}

const char* ShareMemoryGetName(HSHAREMEM hShareMem)
{
    ShareMemory* pShareMem = HandleToObject(hShareMem);
    if (pShareMem)
    {
        return pShareMem->GetName();
    }
    return NULL;
}

bool ShareMemoryIsValid(HSHAREMEM hShareMem)
{
    ShareMemory* pShareMem = HandleToObject(hShareMem);
    if (pShareMem)
    {
        return pShareMem->IsValid();
    }
    return false;
}

UINT32 ShareMemoryGetSize(HSHAREMEM hShareMem)
{
    ShareMemory* pShareMem = HandleToObject(hShareMem);
    if (pShareMem)
    {
        return pShareMem->GetSize();
    }
    return 0;
}

unsigned char* ShareMemoryGetData(HSHAREMEM hShareMem)
{
    ShareMemory* pShareMem = HandleToObject(hShareMem);
    if (pShareMem)
    {
        return pShareMem->GetData();
    }
    return NULL;
}