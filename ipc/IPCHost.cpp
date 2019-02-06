#include "IPCHost.h"
#include "base/asynchronous_task_dispatch.h"

#include <Windows.h>
#include <sstream>
#include <algorithm>
#include <Strsafe.h>
#include <algorithm>
#include <memory>

std::shared_ptr<IPCHost> IPCHost::inst;

void ChannelSessionThread::SetOwner(IPCHost* owner)
{
    m_pIPCSession = owner;
}


void ChannelSessionThread::StartThread()
{
	if (t_.joinable())
		return;

	stop_ = false;
	std::thread t(&ChannelSessionThread::run, this);
	t_ = std::move(t);
}

void ChannelSessionThread::join()
{
	if (t_.joinable())
		t_.join();
}

UINT ChannelSessionThread::run()
{
    while (!stop_)
    {
        if (m_pIPCSession)
        {
            m_pIPCSession->RunRecvThread();
        }
    }
    return 0;
}

IPCHost::IPCHost()
    : m_bInit(false)
    , m_hChannel(NULL)
    , m_nPort(IPC_PORT)
    , m_nTimeOut(1000)
{
    //Init();
}

IPCHost::~IPCHost()
{
    UnRegisterAllIPCMsgReceiver();
    Stop();
    IPCCleanup();
}

void IPCHost::Init()
{
    if (!m_bInit)
    {
        InitIPC(IPC_APP_NAME);
        Start();

        m_bInit = true;
    }
}

void IPCHost::InitIPC(const char* name)
{
    if (!IPCIsActive())
    {
        std::stringstream strStream;
        strStream << name << "_" << GetCurrentProcessId();
        IPCStartup(strStream.str().c_str());
    }
}

void IPCHost::Start()
{
    m_hChannel = ChannelCreate(m_nPort);
    if (m_hChannel)
    {
        m_IPCSessionThread.SetOwner(this);
        m_IPCSessionThread.StartThread();
    }
}

void IPCHost::Stop()
{
    if (m_hChannel)
    {
        ChannelDestroy(m_hChannel);
        m_IPCSessionThread.StopThread();
        m_IPCSessionThread.join();
        m_hChannel = NULL;
    }
}

void IPCHost::RunRecvThread()
{
    int ret = ChannelWaitForEvent(m_hChannel, m_nTimeOut);
    switch (ret)
    {
    case CHANNEL_EVENT_RECV:
        AsyncTaskDispatch::main_thread()->post_task(
            make_closure(&IPCHost::OnRecvPacket, this->shared_from_this()), true
            );
        break;
    case CHANNEL_EVENT_DISCONNECT:
        AsyncTaskDispatch::main_thread()->post_task(
            make_closure(&IPCHost::OnDisconnect, this->shared_from_this()), true
            );
        break;
    }
}

void IPCHost::OnRecvPacket()
{
    while (1)
    {
        PSharePacket pPacket = ChannelGetRecvPacket(m_hChannel);
        if (pPacket)
        {
            LPBYTE pData = pPacket->Data.BodyBuffer;
            UINT32 dwSize = pPacket->Data.BodyBufferLen;
            UINT16 uiMsg = pPacket->Header.uiProtocolID;

            bool bDeal = false;
            std::shared_ptr<BYTE> lpReturnData;
			UINT32 dwReturnSize = 0;
            //¥¶¿Ìpackage
            for (std::vector<IPCReceiver*>::iterator it = m_vecMsgReceiver.begin();
                it != m_vecMsgReceiver.end();
                it++
                )
            {
                if ((bDeal = (*it)->OnIPCMessage(uiMsg, pData, dwSize, lpReturnData, &dwReturnSize)))
                {
                    break;
                }
            }

            if (pPacket->Header.PacketType == SHARE_PCK_REQUEST_SYNC)
            {
                ChannelSendResponse(m_hChannel, pPacket->Header.SrcName, pPacket->Header.uiProtocolID, lpReturnData.get(), dwReturnSize, pPacket->Header.Seq);
            }

            DestroySharePacket(pPacket);
        }
        else
        {
            break;
        }
    }
}

void IPCHost::OnDisconnect()
{
    for (std::vector<IPCReceiver*>::iterator it = m_vecMsgReceiver.begin();
        it != m_vecMsgReceiver.end();
        it++
        )
    {
        (*it)->OnIPCDisconnect();
    }
}

int IPCHost::SendIPCMessage(DWORD dwDstProcessID, UINT16 uiMsgID, LPBYTE lpData, UINT32 iLen)
{
    char szDstIPCName[IPC_NAME_MAX] = { 0 };
    StringCchPrintfA(szDstIPCName, _countof(szDstIPCName), "%s_%u", IPC_APP_NAME, dwDstProcessID);
    return ChannelSendRequest(m_hChannel, szDstIPCName, uiMsgID, (unsigned char*)lpData, iLen);
}

int IPCHost::SendIPCMessageSync(DWORD dwDstProcessID, UINT16 uiMsgID, LPBYTE lpData, UINT32 iLen, PBYTE *lpResponseData, UINT32 *lpdwResponseDataSize, DWORD dwTimeOut)
{
    char szDstIPCName[IPC_NAME_MAX] = { 0 };
    StringCchPrintfA(szDstIPCName, _countof(szDstIPCName), "%s_%u", IPC_APP_NAME, dwDstProcessID);
    PSharePacket pResponsePacket = NULL;
    int iSeq = ChannelSendRequestSync(m_hChannel, szDstIPCName, uiMsgID, (unsigned char*)lpData, iLen, &pResponsePacket, dwTimeOut);

    _ASSERT(iSeq != 0 && iSeq != -1);
    if (iSeq != -1)
    {
        _ASSERTE(pResponsePacket != NULL);
    }

    if (pResponsePacket)
    {
        if (lpdwResponseDataSize || lpResponseData)
        {
            auto buff_len = pResponsePacket->Data.BodyBufferLen;

            if (lpdwResponseDataSize)
                *lpdwResponseDataSize = buff_len;
            if (lpResponseData)
            {
                *lpResponseData = (PBYTE)malloc(buff_len);
                memcpy(*lpResponseData, pResponsePacket->Data.BodyBuffer, buff_len);
            }
        }

        DestroySharePacket(pResponsePacket);
    }

    return iSeq;
}

int IPCHost::SendIPCMessageSync(DWORD dwDstProcessID, UINT16 uiMsgID, LPBYTE lpData, UINT32 iLen, PBYTE lpResponseData, UINT32 dwRespBufSize, UINT32 *lpdwCopyedSize, DWORD dwTimeOut /*= INFINITE*/)
{
    char szDstIPCName[IPC_NAME_MAX] = { 0 };
    StringCchPrintfA(szDstIPCName, _countof(szDstIPCName), "%s_%u", IPC_APP_NAME, dwDstProcessID);
    PSharePacket pResponsePacket = NULL;
    int iSeq = ChannelSendRequestSync(m_hChannel, szDstIPCName, uiMsgID, (unsigned char*)lpData, iLen, &pResponsePacket, dwTimeOut);

    _ASSERT(iSeq != 0 && iSeq != -1);
    if (iSeq != -1)
    {
        _ASSERTE(pResponsePacket != NULL);
    }

    if (pResponsePacket)
    {
        if (dwRespBufSize >0 && lpResponseData)
        {
            auto len = std::min((UINT32)dwRespBufSize, pResponsePacket->Data.BodyBufferLen);
            
            if (lpdwCopyedSize)
                *lpdwCopyedSize = len;
            memcpy(lpResponseData, pResponsePacket->Data.BodyBuffer, len);
        }

        DestroySharePacket(pResponsePacket);
    }

    return iSeq;
}

void IPCHost::RegisterIPCMsgReceiver(IPCReceiver* pReceiver)
{
    if (pReceiver)
        m_vecMsgReceiver.push_back(pReceiver);
}

void IPCHost::UnRegisterIPCMsgReceiver(IPCReceiver* pReceiver)
{
    if (pReceiver)
    {
        std::vector<IPCReceiver*>::iterator it = std::find(m_vecMsgReceiver.begin(), m_vecMsgReceiver.end(), pReceiver);
        if (it != m_vecMsgReceiver.end())
        {
            m_vecMsgReceiver.erase(it);
        }
    }
}

void IPCHost::UnRegisterAllIPCMsgReceiver()
{
    m_vecMsgReceiver.clear();
}
