#include "ChannelManager.h"

#include <algorithm>
#include <crtdbg.h>

using namespace std;
using namespace System;
using namespace IPC;

UINT SendThread::run()
{
    while (!stop_)
    {
        ChannelManager::Instance().SendWorkThread();
    }
    return 0;
}

IPC::SendThread::~SendThread()
{
    stop();
	if (t_.joinable())
		t_.join();
}


void IPC::SendThread::start()
{
	if (t_.joinable())
	{
		return;
	}

	std::thread t(&IPC::SendThread::run, this);
	t_ = std::move(t);
}

void IPC::SendThread::stop()
{
    stop_ = true;
}


void IPC::SendThread::join()
{
	if (t_.joinable())
		t_.join();
}

UINT RecvThread::run()
{
    while (!stop_)
    {
        ChannelManager::Instance().RecvWorkThread();
    }
    return 0;
}

IPC::RecvThread::~RecvThread()
{
    stop();
	if (t_.joinable())
		t_.join();
}


void IPC::RecvThread::start()
{
	if (t_.joinable())
		return;

	std::thread t(&IPC::RecvThread::run, this);
	t_ = std::move(t);
}

void IPC::RecvThread::stop()
{
    stop_ = true;
}


void IPC::RecvThread::join()
{
	if (t_.joinable())
		t_.join();
}

ChannelManager::ChannelManager() :
m_bInit(false),
m_nMaxChannelPort(1)
{
    m_semSend.Create(NULL);
    memset(m_szName, 0, IPC_NAME_MAX + 1);
    memset(m_channels, 0, sizeof(m_channels));
}

ChannelManager::~ChannelManager()
{
    if (m_bInit)
    {
        Stop();
        WaitForEnd();
    }
}

ChannelManager& ChannelManager::Instance()
{
    static ChannelManager instance;
    return instance;
}

UINT16 ChannelManager::AddChannel(Channel* pChannel)
{
    if (!m_bInit || !pChannel)
        return 0;

    m_csChannels.Lock();
    CriticalSectionGuard guard(m_csChannels);

    for (int i = 0; i < IPC_CHANNEL_MAX; i++)
    {
        if (!m_channels[i])
        {
            m_channels[i] = pChannel;
            UINT16 port = i + 1;
            if (m_nMaxChannelPort < port)
            {
                m_nMaxChannelPort = port;
            }
            return port;
        }
    }
    return 0;
}

UINT16 ChannelManager::AddChannel(Channel* pChannel, UINT16 nPort)
{
    if (nPort == 0)
        return AddChannel(pChannel);

    if (!m_bInit || !pChannel || nPort > IPC_CHANNEL_MAX)
        return 0;

    m_csChannels.Lock();
    CriticalSectionGuard guard(m_csChannels);

    if (!m_channels[nPort - 1])
    {
        m_channels[nPort - 1] = pChannel;
        if (m_nMaxChannelPort < nPort)
        {
            m_nMaxChannelPort = nPort;
        }
        return nPort;
    }
    return 0;
}

bool ChannelManager::RemoveChannel(UINT16 nPort)
{
    if (!m_bInit || nPort > IPC_CHANNEL_MAX)
        return false;

    m_csChannels.Lock();
    CriticalSectionGuard guard(m_csChannels);

    if (m_channels[nPort - 1])
    {
        m_channels[nPort - 1] = NULL;
        return true;
    }
    return false;
}

Channel* ChannelManager::GetChannel(UINT16 port)
{
    if (!m_bInit || port > IPC_CHANNEL_MAX)
        return NULL;

    m_csChannels.Lock();
    CriticalSectionGuard guard(m_csChannels);

    return m_channels[port - 1];
}

bool ChannelManager::Start(const char* pszName)
{
    if (m_bInit || !pszName)
        return false;
    size_t n = strnlen(pszName, IPC_NAME_MAX + 1);
    if (n > IPC_NAME_MAX || n <= 0)
        return false;
    m_csManager.Lock();
    CriticalSectionGuard csguard(m_csManager);
    memset(m_szName, 0, IPC_NAME_MAX + 1);
    strncpy(m_szName, pszName, IPC_NAME_MAX);
    string strName = m_szName;
    strName += "_RECV_DATA_QUEUE";
    if (!m_packetQueueRecv.Create(strName.c_str()))
    {
        _ASSERT(0);
        return false;
    }
    strName = m_szName;
    strName += "_ACTIVE_MUTEX";
    if (!m_mutexActive.Create(strName.c_str()))
    {
        m_packetQueueRecv.Disconnect();
        _ASSERT(0);
        return false;
    }
    m_threadRecv.start();
    m_threadSend.start();
    m_bInit = true;
    return true;
}

bool ChannelManager::Stop()
{
    if (!m_bInit)
        return false;
    m_csManager.Lock();
    CriticalSectionGuard csguard(m_csManager);
    memset(m_szName, 0, IPC_NAME_MAX + 1);
    m_mutexActive.Release();
    m_threadRecv.stop();
	m_threadRecv.join();
    m_threadSend.stop();
	m_threadSend.join();
    ClearChannels();
    ClearPacketQueueCacheMap();
    ClearSendPacketList();
    m_semSend.Active();
    m_packetQueueRecv.ActiveSend();
    m_bInit = false;
    return true;
}

bool ChannelManager::TryStop()
{
    if (!m_bInit)
        return false;
    m_csManager.Lock();
    CriticalSectionGuard csguard(m_csManager);
    m_csChannels.Lock();
    CriticalSectionGuard guard(m_csChannels);
    for (int i = 0; i < IPC_CHANNEL_MAX; i++)
    {
        if (m_channels[i])
        {
            return false;
        }
    }
    return Stop();
}

int ChannelManager::WaitForEnd()
{
    m_threadRecv.stop();
	m_threadRecv.join();
    m_threadSend.stop();
	m_threadSend.join();

    m_packetQueueRecv.Disconnect();
    return 0;
}

UINT32 ChannelManager::GenerateSeq()
{
    m_csGenSeq.Lock();
    CriticalSectionGuard guard(m_csGenSeq);
    static UINT32 nSeq = 0;
    if (nSeq == UINT32_MAX)
    {
        nSeq = 1;
    }
    else
    {
        nSeq++;
    }
    return nSeq;
}

bool ChannelManager::IsActive()
{
    return m_bInit;
}

const char* ChannelManager::GetName() const
{
    if (strnlen(m_szName, IPC_NAME_MAX) > 0)
    {
        return m_szName;
    }
    return NULL;
}

bool ChannelManager::SendPacket(SharePacket* pPacket)
{
    m_csSend.Lock();
    CriticalSectionGuard guard(m_csSend);
    bool active = m_packetListSend.empty();
    m_packetListSend.push_back(pPacket);
    if (active)
        m_semSend.Active();
    return true;
}

void ChannelManager::SendWorkThread()
{
    static DWORD dwBegin = GetTickCount();
    int ret = m_semSend.TryLock(3000);
    if (ret == WAIT_OBJECT_0)
    {
        OnSendPacket();
    }

    if (ret != WAIT_OBJECT_0 || GetTickCount() - dwBegin >= 3000)
    {
        CheckChannelsActive();
        dwBegin = GetTickCount();
    }
}

void ChannelManager::RecvWorkThread()
{
    if (m_packetQueueRecv.WaitRecv(1000) == WAIT_OBJECT_0)
    {
        OnRecvPacket();
    }
}

void ChannelManager::OnRecvPacket()
{
	UINT32 dwNum = 0;
    do
    {
        m_packetQueueRecv.Lock();
        dwNum = m_packetQueueRecv.GetPacketNum();
        bool bPopData = false;
        SharePacket* pRecvPacket = NULL;
        if (dwNum > 0)
        {
            pRecvPacket = CreateSharePacket();
            bPopData = m_packetQueueRecv.PopData(pRecvPacket);
            m_packetQueueRecv.Unlock();
        }
        else
        {
            
            m_packetQueueRecv.Unlock();
            break;
        }

        bool bDispatch = false;
        if (bPopData && pRecvPacket)
        {
            m_csPacketQueueCacheMap.Lock();
            m_csChannels.Lock();
            Channel* pChannel = GetChannel(pRecvPacket->Header.Port);

            if (pChannel)
            {
                char szName[IPC_NAME_MAX + 1];
                memset(szName, 0, IPC_NAME_MAX + 1);
                strncpy(szName, pRecvPacket->Header.SrcName, IPC_NAME_MAX);
                string strSrcName = szName;
                UINT16 nPort = pRecvPacket->Header.Port;

                pChannel->OnRecvPacket(pRecvPacket);

                map<std::string, PairPacketQueue>::iterator iterFind;
                iterFind = m_packetQueueCacheMap.find(strSrcName);
                if (iterFind == m_packetQueueCacheMap.end())
                {
                    set<UINT16> setPort;
                    setPort.insert(nPort);
                    SharePacketQueue* pSharePacketQueue = NULL;
                    PairPacketQueue pairPacketQueue = make_pair(pSharePacketQueue, setPort);
                    m_packetQueueCacheMap.insert(map<string, PairPacketQueue>::value_type(strSrcName, pairPacketQueue));
                }

                bDispatch = true;
            }
            m_csChannels.Unlock();
            m_csPacketQueueCacheMap.Unlock();
        }
        if (!bDispatch)
        {
            DestroySharePacket(pRecvPacket);
        }
    } while (dwNum > 0);
}

void ChannelManager::OnSendPacket()
{
    std::list<SharePacket*> packetListSend;

    m_csSend.Lock();
    if (!m_packetListSend.empty())
    {
        packetListSend.swap(m_packetListSend);
    }
    m_csSend.Unlock();

    for (std::list<SharePacket*>::iterator it = packetListSend.begin();
        it != packetListSend.end();
        it++
        )
    {
        WritePacket(*it);
        DestroySharePacket(*it);
    }
}

bool ChannelManager::WritePacket(PSharePacket pPacket, bool bCheckActive, UINT nTimeout)
{
    string strDestName;
    char szName[IPC_NAME_MAX + 1];
    memset(szName, 0, IPC_NAME_MAX + 1);
    strncpy(szName, pPacket->Header.DestName, IPC_NAME_MAX);
    strDestName = szName;

    m_csPacketQueueCacheMap.Lock();
    CriticalSectionGuard guard(m_csPacketQueueCacheMap);

    map<std::string, PairPacketQueue>::iterator iterFind;
    iterFind = m_packetQueueCacheMap.find(strDestName);

    if (bCheckActive)
    {
        if (!CheckActive(strDestName.c_str()))
        {
            if (iterFind != m_packetQueueCacheMap.end())
            {
                PairPacketQueue pairPacketQueue = iterFind->second;
                SharePacketQueue* pSharePacketQueue = pairPacketQueue.first;
                if (pSharePacketQueue)
                {
                    pSharePacketQueue->Disconnect();
                    delete pSharePacketQueue;
                    pairPacketQueue.first = NULL;
                }
                m_packetQueueCacheMap.erase(iterFind);

                pairPacketQueue.second.insert(pPacket->Header.Port);
                m_csChannels.Lock();
                set<UINT16>::iterator iterPort;
                for (iterPort = pairPacketQueue.second.begin(); iterPort != pairPacketQueue.second.end(); ++iterPort)
                {
                    Channel* pChannel = GetChannel(*iterPort);
                    if (pChannel)
                    {
                        PChannelAddr pChannelAddr = new ChannelAddr();
                        memset(pChannelAddr->Name, 0, IPC_NAME_MAX);
                        strncpy(pChannelAddr->Name, pPacket->Header.DestName, IPC_NAME_MAX);
                        pChannelAddr->Port = *iterPort;
                        pChannel->OnDisconnect(pChannelAddr);
                    }
                }
                m_csChannels.Unlock();

            }
            else
            {
                m_csChannels.Lock();
                Channel* pChannel = GetChannel(pPacket->Header.Port);
                if (pChannel)
                {
                    PChannelAddr pChannelAddr = new ChannelAddr();
                    memset(pChannelAddr->Name, 0, IPC_NAME_MAX);
                    strncpy(pChannelAddr->Name, pPacket->Header.DestName, IPC_NAME_MAX);
                    pChannelAddr->Port = pPacket->Header.Port;
                    pChannel->OnDisconnect(pChannelAddr);
                }
                m_csChannels.Unlock();
            }

            return false;
        }
    }

    SharePacketQueue* pSharePacketQueueDest = NULL;
    if (iterFind != m_packetQueueCacheMap.end())
    {
        pSharePacketQueueDest = iterFind->second.first;
        iterFind->second.second.insert(pPacket->Header.Port);
    }

    if (!pSharePacketQueueDest)
    {
        pSharePacketQueueDest = new SharePacketQueue();
        string strDestRecvQueueName;
        strDestRecvQueueName = strDestName + "_RECV_DATA_QUEUE";
        if (pSharePacketQueueDest->Connect(strDestRecvQueueName.c_str()))
        {
            if (iterFind != m_packetQueueCacheMap.end())
            {
                iterFind->second.first = pSharePacketQueueDest;
            }
            else
            {
                set<UINT16> setPort;
                setPort.insert(pPacket->Header.Port);
                PairPacketQueue pairPacketQueue = make_pair(pSharePacketQueueDest, setPort);
                m_packetQueueCacheMap.insert(map<string, PairPacketQueue>::value_type(strDestName, pairPacketQueue));
            }
        }
        else
        {
            delete pSharePacketQueueDest;
            if (iterFind != m_packetQueueCacheMap.end())
            {
                m_packetQueueCacheMap.erase(iterFind);
            }
            return false;
        }
    }

    bool result = false;
    if (pSharePacketQueueDest)
    {
        if (nTimeout == INFINITE)
        {
            pSharePacketQueueDest->Lock();
            result = pSharePacketQueueDest->PushData(pPacket) > 0;
            _ASSERT_EXPR(result, L"SharePacketQueue::PushData 失败,请通知pika！！！");
            pSharePacketQueueDest->Unlock();
        }
        else
        {
            if (pSharePacketQueueDest->TryLock(nTimeout) != WAIT_TIMEOUT)
            {
                result = pSharePacketQueueDest->PushData(pPacket) > 0;
                _ASSERT_EXPR(result, L"SharePacketQueue::PushData 失败,请通知pika！！！");
                pSharePacketQueueDest->Unlock();
            }
        }
    }
    return result;
}

void ChannelManager::ClearChannels()
{
    m_csChannels.Lock();
    CriticalSectionGuard guard(m_csChannels);
    for (int i = 0; i < IPC_CHANNEL_MAX; i++)
    {
        if (m_channels[i])
        {
            m_channels[i]->Close();
            m_channels[i] = NULL;
        }
    }
}

void ChannelManager::ClearSendPacketList()
{
    m_csSend.Lock();
    CriticalSectionGuard guard(m_csSend);
    list<SharePacket*>::iterator iter, iterEnd = m_packetListSend.end();
    for (iter = m_packetListSend.begin(); iter != iterEnd; ++iter)
    {
        SharePacket* pSharePacket = *iter;
        DestroySharePacket(pSharePacket);
    }
    m_packetListSend.clear();
}

void ChannelManager::ClearPacketQueueCacheMap()
{
    m_csPacketQueueCacheMap.Lock();
    CriticalSectionGuard guardCacheMap(m_csPacketQueueCacheMap);
    map<string, PairPacketQueue>::iterator iter, iterEnd = m_packetQueueCacheMap.end();
    for (iter = m_packetQueueCacheMap.begin(); iter != iterEnd; ++iter)
    {
        PairPacketQueue pairPacketQueue = iter->second;
        SharePacketQueue* pSharePacketQueue = pairPacketQueue.first;
        if (pSharePacketQueue)
        {
            pSharePacketQueue->Disconnect();
            delete pSharePacketQueue;
            pairPacketQueue.first = NULL;
        }
    }
    m_packetQueueCacheMap.clear();
}

void ChannelManager::CheckChannelsActive()
{
    m_csPacketQueueCacheMap.Lock();
    CriticalSectionGuard guardCacheMap(m_csPacketQueueCacheMap);
    map<std::string, PairPacketQueue>::iterator iter;
    for (iter = m_packetQueueCacheMap.begin(); iter != m_packetQueueCacheMap.end();)
    {
        string destName = iter->first;
        if (!CheckActive(destName.c_str()))
        {
            PairPacketQueue pairPacketQueue = iter->second;

            iter = m_packetQueueCacheMap.erase(iter);

            SharePacketQueue* pSharePacketQueue = pairPacketQueue.first;
            if (pSharePacketQueue)
            {
                pSharePacketQueue->Disconnect();
                delete pSharePacketQueue;
                pairPacketQueue.first = NULL;
            }

            m_csChannels.Lock();
            set<UINT16>::iterator iterPort;
            for (iterPort = pairPacketQueue.second.begin(); iterPort != pairPacketQueue.second.end(); ++iterPort)
            {
                Channel* pChannel = GetChannel(*iterPort);
                if (pChannel)
                {
                    PChannelAddr pChannelAddr = new ChannelAddr();
                    memset(pChannelAddr->Name, 0, IPC_NAME_MAX);
                    strncpy(pChannelAddr->Name, destName.c_str(), IPC_NAME_MAX);
                    pChannelAddr->Port = *iterPort;
                    pChannel->OnDisconnect(pChannelAddr);
                }
            }
            m_csChannels.Unlock();

            continue;
        }
        ++iter;
    }
}

bool ChannelManager::CheckActive(const char* pszName)
{
    if (pszName)
    {
        string strName = pszName;
        strName += "_ACTIVE_MUTEX";
        Mutex mutexDestActive;
        if (mutexDestActive.Open(strName.c_str()))
        {
            mutexDestActive.Release();
            return true;
        }
    }

    return false;
}