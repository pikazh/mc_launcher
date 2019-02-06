#ifndef __CHANNEL_MANAGER_H__
#define __CHANNEL_MANAGER_H__

#include <windows.h>
#include <string>
#include <list>
#include <map>
#include <set>
#include <thread>

#include "cs.hpp"
#include "mutex.hpp"
#include "semaphore.hpp"

#include "./Channel.h"
#include "./SharePacketQueue.h"

namespace IPC
{

class SendThread 
{
public:
    SendThread() = default;
    virtual ~SendThread();
public:
	void start();
    void stop();
	void join();
protected:
	virtual UINT run();
private:
    volatile bool stop_ = false;
	std::thread t_;
};

class RecvThread
{
public:
    RecvThread() = default;
    virtual ~RecvThread();
public:
	void start();
	void stop();
	void join();
protected:
	virtual UINT run();
private:
    volatile bool stop_ = false;
	std::thread t_;
};

class ChannelManager
{
protected:
    ChannelManager();
    ~ChannelManager();
public:
    static ChannelManager& Instance();
public:
    UINT16 AddChannel(Channel* pChannel);
    UINT16 AddChannel(Channel* pChannel, UINT16 nPort);
    bool RemoveChannel(UINT16 port);
    Channel* GetChannel(UINT16 port);
    bool Start(const char* pszName);
    bool Stop();
    bool TryStop();
    int WaitForEnd();
	UINT32 GenerateSeq();
    bool IsActive();
    const char* GetName() const;
    bool CheckActive(const char* pszName);
public:
    bool SendPacket(SharePacket* pPacket);
    bool WritePacket(PSharePacket pPacket, bool bCheckActive = false, UINT nTimeout = INFINITE);
public:
    void SendWorkThread();
    void RecvWorkThread();
protected:
    void OnRecvPacket();
    void OnSendPacket();
    void ClearChannels();
    void ClearSendPacketList();
    void ClearPacketQueueCacheMap();
    void CheckChannelsActive();
protected:
    char m_szName[IPC_NAME_MAX + 1];
    bool m_bInit;
    SendThread m_threadSend;
    RecvThread m_threadRecv;
    std::string m_strName;
    SharePacketQueue m_packetQueueRecv;
    System::CriticalSection m_csManager;
    System::CriticalSection m_csSend;
    System::CriticalSection m_csChannels;
    System::CriticalSection m_csPacketQueueCacheMap;
    System::CriticalSection m_csGenSeq;
    System::Semaphore m_semSend;
    System::Mutex m_mutexActive;
    std::list<SharePacket*> m_packetListSend;
    UINT16 m_nMaxChannelPort;
    Channel* m_channels[IPC_CHANNEL_MAX];
    typedef std::pair<SharePacketQueue*, std::set<UINT16>> PairPacketQueue;
    std::map<std::string, PairPacketQueue> m_packetQueueCacheMap;
};

}

#endif