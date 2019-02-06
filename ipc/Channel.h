#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#include <string>
#include <list>
#include <map>
#include "ipc.h"

#include "semaphore.hpp"
#include "mutex.hpp"
#include "cs.hpp"
#include "SharePacketQueue.h"

namespace IPC
{

typedef struct tagSyncRequest
{
    SharePacket* pResponsePacket;
    System::Semaphore semRecv;
} SyncRequest, *PSyncRequest;

typedef struct tagConnectData
{
    UINT16 port;
    SharePacketQueue* packetQueue;
} ConnectData, *PConnectData;

class ChannelManager;

class Channel
{
    friend class ChannelManager;
protected:
    Channel();
public:
    ~Channel();
public:
    static Channel* Create(UINT16 nPort);
    bool Destroy();
public:
    bool Close();
    int Send(const char* pszName, UINT16 uiPortocolID, byte* pBuffer, UINT32 len);
    int SendRequest(const char* pszName, UINT16 uiPortocolID, byte* pBuffer, UINT32 len);
    int SendRequestSync(const char* pszName, UINT16 uiPortocolID, byte* pBuffer, UINT32 len, PSharePacket& pResponsePacket, UINT nTimeout);
    int SendResponse(const char* pszName, UINT16 uiPortocolID, byte* pBuffer, UINT32 len, UINT32 nSeq);
    int WaitForEvent(UINT nTimeout);
    PSharePacket GetRecvPacket();
    bool GetDisconnect(PChannelAddr pChannelAddr);
protected:
    void OnRecvPacket(PSharePacket pPacket);
    void OnDisconnect(PChannelAddr pChannelAddr);
protected:
    void Clear();
    void ClearSyncRequest();
    void RemoveSyncRequest(UINT32 nSeq);
    void DestroySyncRequest(SyncRequest* pRequest);
    bool SetPacketHeader(UINT16 uiProtocolID, const char* pszName, UINT32 nSeq, PSharePacketHeader pHeader);
    bool SetPacketBuffer(PSharePacketData pData, byte* pBuffer, UINT32 len);
    bool CheckNameValid(const char* pszName);
    bool ProcessMessage();
protected:
    UINT16 m_port;
    bool m_bActived;
    System::Semaphore m_semDisconnect;
    System::Semaphore m_semRecv;
    System::Semaphore m_semClose;
    System::CriticalSection m_csDisconnect;
    System::CriticalSection m_csRecv;
    System::CriticalSection m_csSyncRequest;
    System::CriticalSection m_csConnect;
    std::list<PSharePacket> m_packetListRecv;
    std::list<PChannelAddr> m_channelListDisconnect;
    std::map<UINT32, PSyncRequest> m_mapRequest;
};

}

#endif