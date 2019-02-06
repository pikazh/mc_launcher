#include "Channel.h"
#include "ChannelManager.h"
#include "memory_helper.h"

#include <signal.h>

using namespace std;
using namespace IPC;
using namespace System;

Channel::Channel() :
m_port(0),
m_bActived(false)
{
    m_semRecv.Create(NULL);
    m_semDisconnect.Create(NULL);
    m_semClose.Create(NULL);
}

Channel::~Channel()
{
    Close();
    Clear();
}

Channel* Channel::Create(UINT16 nPort)
{
    Channel* pChannel = new Channel();
    UINT16 ret = ChannelManager::Instance().AddChannel(pChannel, nPort);
    if (ret)
    {
        pChannel->m_port = ret;
        pChannel->m_bActived = true;
        return pChannel;
    }
    else
    {
        delete pChannel;
        return NULL;
    }
}

bool Channel::Destroy()
{
    Close();
    delete this;
    return true;
}

bool Channel::Close()
{
    if (m_bActived)
    {
        m_bActived = false;
        bool ret = ChannelManager::Instance().RemoveChannel(m_port);
        ClearSyncRequest();
        m_semClose.Active();
        return ret;
    }
    return false;
}

int Channel::Send(const char* pszName, UINT16 uiPortocolID, byte* pBuffer, UINT32 len)
{
    if (!m_bActived || !CheckNameValid(pszName))
        return 0;
    PSharePacket pSharePacket = CreateSharePacket();
    pSharePacket->Header.PacketType = SHARE_PCK_NORMAL;
    int nSeq = ChannelManager::Instance().GenerateSeq();
    SetPacketHeader(uiPortocolID, pszName, nSeq, &pSharePacket->Header);
    SetPacketBuffer(&pSharePacket->Data, pBuffer, len);
    ChannelManager::Instance().SendPacket(pSharePacket);
    return nSeq;
}

int Channel::SendRequest(const char* pszName, UINT16 uiPortocolID, byte* pBuffer, UINT32 len)
{
    if (!m_bActived || !CheckNameValid(pszName))
        return 0;
    PSharePacket pSharePacket = CreateSharePacket();
    pSharePacket->Header.PacketType = SHARE_PCK_REQUEST;
    int nSeq = ChannelManager::Instance().GenerateSeq();
    SetPacketHeader(uiPortocolID, pszName, nSeq, &pSharePacket->Header);
    SetPacketBuffer(&pSharePacket->Data, pBuffer, len);
    ChannelManager::Instance().SendPacket(pSharePacket);
    return nSeq;
}

int Channel::SendRequestSync(const char* pszName, UINT16 uiPortocolID, byte* pBuffer, UINT32 len, PSharePacket& pResponsePacket, UINT nTimeout)
{
    if (!m_bActived || !CheckNameValid(pszName) || pResponsePacket)
        return 0;
    PSharePacket pSharePacket = CreateSharePacket();
    pSharePacket->Header.PacketType = SHARE_PCK_REQUEST_SYNC;
    int nSeq = ChannelManager::Instance().GenerateSeq();
    SetPacketHeader(uiPortocolID, pszName, nSeq, &pSharePacket->Header);
    SetPacketBuffer(&pSharePacket->Data, pBuffer, len);
    SyncRequest* pRequest = new SyncRequest();
    pRequest->pResponsePacket = NULL;
    pRequest->semRecv.Create(NULL);
    m_csSyncRequest.Lock();
    if (m_bActived)
    {
        m_mapRequest.insert(map<UINT16, SyncRequest*>::value_type(nSeq, pRequest));
    }
    m_csSyncRequest.Unlock();


    if (!ChannelManager::Instance().SendPacket(pSharePacket))
    {
        RemoveSyncRequest(nSeq);
        DestroySyncRequest(pRequest);
        return -1;
    }

    if (pRequest->semRecv.TryLock(nTimeout) != WAIT_OBJECT_0)
    {
        RemoveSyncRequest(nSeq);
        DestroySyncRequest(pRequest);
        return -1;
    }

    if (!pRequest->pResponsePacket)
    {
        DestroySyncRequest(pRequest);
        return -1;
    }
    else
    {
        pResponsePacket = pRequest->pResponsePacket;
        pRequest->pResponsePacket = NULL;
    }

    DestroySyncRequest(pRequest);
    return nSeq;
}

int Channel::SendResponse(const char* pszName, UINT16 uiPortocolID, byte* pBuffer, UINT32 len, UINT32 nSeq)
{
    if (!m_bActived || !CheckNameValid(pszName))
        return 0;
    PSharePacket pSharePacket = CreateSharePacket();
    pSharePacket->Header.PacketType = SHARE_PCK_RESPONSE;
    SetPacketHeader(uiPortocolID, pszName, nSeq, &pSharePacket->Header);
    SetPacketBuffer(&pSharePacket->Data, pBuffer, len);
    ChannelManager::Instance().SendPacket(pSharePacket);
    return nSeq;
}

int Channel::WaitForEvent(UINT nTimeout)
{
    if (!m_bActived)
    {
        return CHANNEL_EVENT_CLOSE;
    }
    HANDLE handles[3];
    handles[0] = (HANDLE)m_semRecv.Handle();
    handles[1] = (HANDLE)m_semDisconnect.Handle();
    handles[2] = (HANDLE)m_semClose.Handle();
    DWORD ret = WaitForMultipleObjects(3, handles, FALSE, nTimeout);
    switch (ret)
    {
    case WAIT_OBJECT_0:
        return CHANNEL_EVENT_RECV;
    case WAIT_OBJECT_0 + 1:
        return CHANNEL_EVENT_DISCONNECT;
    case WAIT_OBJECT_0 + 2:
        return CHANNEL_EVENT_CLOSE;
    case WAIT_TIMEOUT:
        return CHANNEL_EVENT_TIMEOUT;
    };
    return CHANNEL_EVENT_ERROR;
}

PSharePacket Channel::GetRecvPacket()
{
    m_csRecv.Lock();
    CriticalSectionGuard guard(m_csRecv);
    if (!m_packetListRecv.empty())
    {
        PSharePacket pPacket = m_packetListRecv.front();
        m_packetListRecv.pop_front();
        return pPacket;
    }
    return NULL;
}

bool Channel::GetDisconnect(PChannelAddr pChannelAddr)
{
    if (!pChannelAddr)
        return false;
    m_csDisconnect.Lock();
    CriticalSectionGuard guard(m_csDisconnect);
    if (m_channelListDisconnect.size() > 0)
    {
        PChannelAddr pAddr = m_channelListDisconnect.front();
        m_channelListDisconnect.pop_front();
        if (pAddr)
        {
            memcpy(pChannelAddr, pAddr, sizeof(ChannelAddr));
            delete pAddr;
            return true;
        }
    }
    return false;
}

void Channel::OnRecvPacket(SharePacket* pPacket)
{
    bool bSync = false;
    if (pPacket->Header.PacketType == SHARE_PCK_RESPONSE)
    {
		UINT32 nSeq = pPacket->Header.Seq;
        m_csSyncRequest.Lock();
        map<UINT32, SyncRequest*>::iterator iterFind = m_mapRequest.find(nSeq);
        if (iterFind != m_mapRequest.end())
        {
            SyncRequest* pRequest = iterFind->second;
            if (pRequest)
            {
                pRequest->pResponsePacket = pPacket;
                pRequest->semRecv.Active();
                bSync = true;
            }
            m_mapRequest.erase(iterFind);
        }
        m_csSyncRequest.Unlock();
    }

    if (!bSync)
    {
        m_csRecv.Lock();
        CriticalSectionGuard guard(m_csRecv);
        bool active = m_packetListRecv.empty();
        m_packetListRecv.push_back(pPacket);
        if (active)
            m_semRecv.Active();
    }
}

void Channel::OnDisconnect(PChannelAddr pChannelAddr)
{
    m_csDisconnect.Lock();
    m_channelListDisconnect.push_back(pChannelAddr);
    m_csDisconnect.Unlock();
    m_semDisconnect.Active();
}

void Channel::Clear()
{
    ClearSyncRequest();

    m_csDisconnect.Lock();
    list<PChannelAddr>::iterator iterDisconnect, iterDisconnectEnd = m_channelListDisconnect.end();
    for (iterDisconnect = m_channelListDisconnect.begin(); iterDisconnect != iterDisconnectEnd; ++iterDisconnect)
    {
        PChannelAddr pChannelAddr = *iterDisconnect;
        if (pChannelAddr)
        {
            delete pChannelAddr;
        }
    }
    m_channelListDisconnect.clear();
    m_csDisconnect.Unlock();

    m_csRecv.Lock();
    list<PSharePacket>::iterator iterRecv, iterRecvEnd = m_packetListRecv.end();
    for (iterRecv = m_packetListRecv.begin(); iterRecv != iterRecvEnd; ++iterRecv)
    {
        SharePacket* pSharePacket = *iterRecv;
        if (pSharePacket)
        {
            DestroySharePacket(pSharePacket);
        }
    }
    m_packetListRecv.clear();
    m_csRecv.Unlock();
}

void Channel::ClearSyncRequest()
{
    m_csSyncRequest.Lock();
    map<UINT32, SyncRequest*>::iterator iterSyncRequest, iterSyncRequestEnd = m_mapRequest.end();
    for (iterSyncRequest = m_mapRequest.begin(); iterSyncRequest != iterSyncRequestEnd; ++iterSyncRequest)
    {
        PSyncRequest pRequest = iterSyncRequest->second;
        if (pRequest)
        {
            pRequest->semRecv.Active();
        }
    }
    m_mapRequest.clear();
    m_csSyncRequest.Unlock();
}

void Channel::RemoveSyncRequest(UINT32 nSeq)
{
    m_csSyncRequest.Lock();
    map<UINT32, SyncRequest*>::iterator iterFind = m_mapRequest.find(nSeq);
    if (iterFind != m_mapRequest.end())
    {
        m_mapRequest.erase(iterFind);
    }
    m_csSyncRequest.Unlock();
}

void Channel::DestroySyncRequest(SyncRequest* pRequest)
{
    if (pRequest)
    {
        if (pRequest->pResponsePacket)
        {
            DestroySharePacket(pRequest->pResponsePacket);
        }
        delete pRequest;
    }
}

bool Channel::SetPacketHeader(UINT16 uiProtocolID, const char* pszName, UINT32 nSeq, PSharePacketHeader pHeader)
{
    pHeader->uiProtocolID = uiProtocolID;
    strncpy(pHeader->DestName, pszName, IPC_NAME_MAX);
    strncpy(pHeader->SrcName, ChannelManager::Instance().GetName(), IPC_NAME_MAX);
    pHeader->Port = m_port;
    pHeader->Seq = nSeq;
    return true;
}

bool Channel::SetPacketBuffer(PSharePacketData pData, byte* pBuffer, UINT32 len)
{
    if (pBuffer && len > 0)
    {
        pData->BodyBufferLen = len;
        pData->BodyBuffer = (BYTE*)MemoryPool->alloca(len);
        memcpy(pData->BodyBuffer, pBuffer, len);
        return true;
    }
    return false;
}

bool Channel::CheckNameValid(const char* pszName)
{
    if (!pszName)
        return false;
    size_t n = strnlen(pszName, IPC_NAME_MAX + 1);
    if (n > IPC_NAME_MAX || n <= 0)
        return false;
    return true;
}

bool Channel::ProcessMessage()
{
    MSG msg;
    if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message != WM_QUIT)
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
        else
        {
            return false;
        }
    }
    return true;
}