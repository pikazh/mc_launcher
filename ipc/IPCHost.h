#ifndef __IPC_HOST_
#define __IPC_HOST_

#include "ipc.h"
#include <windows.h>
#include <vector>
#include <memory>
#include <string>
#include <thread>

class IPCHost;
class IPCReceiver;

class ChannelSessionThread
{
public:
    ChannelSessionThread() = default;
    void SetOwner(IPCHost* owner);
	void StartThread();
    void StopThread()
    {
        stop_ = true;
    }
	void join();
private:
    virtual UINT run();
    IPCHost* m_pIPCSession = nullptr;
    volatile bool stop_ = false;
	std::thread t_;
};

enum SwitchThreadId
{
    Switch_OnRecvPacket = 1,
    Switch_OnDisconnect,
};

class IPCHost
    : public std::enable_shared_from_this<IPCHost>
{
	friend class ChannelSessionThread;
public:
    static IPCHost* GetInstance()
    {
        if (!inst)
        {
            inst.reset(new IPCHost);
        }
        return inst.get();
    }

    static void DeleteInstance()
    {
        if (!!inst)
            inst.reset();
    }

    ~IPCHost();

    void Init();

	void RegisterIPCMsgReceiver(IPCReceiver*);
	void UnRegisterIPCMsgReceiver(IPCReceiver*);
	void UnRegisterAllIPCMsgReceiver();
	
	int SendIPCMessage(DWORD dwDstProcessID, UINT16 uiMsgID, LPBYTE lpData, UINT32 iLen);
	int SendIPCMessageSync(DWORD dwDstProcessID, UINT16 uiMsgID, LPBYTE lpData, UINT32 iLen, PBYTE *lpResponseData = NULL, UINT32 *lpdwResponseDataSize = NULL, DWORD dwTimeOut = INFINITE);
    int SendIPCMessageSync(DWORD dwDstProcessID, UINT16 uiMsgID, LPBYTE lpData, UINT32 iLen, PBYTE lpResponseData, UINT32 dwResponse_buffer_size, UINT32 *lpdwCopyedSize, DWORD dwTimeOut = INFINITE);

protected:
    IPCHost();
    

    void OnRecvPacket();
    void OnDisconnect();
    void RunRecvThread();

    void InitIPC(const char* name);
    void Start();
    void Stop();

private:
    ChannelSessionThread m_IPCSessionThread;
	HCHANNEL m_hChannel;
	UINT16 m_nPort;
	UINT m_nTimeOut;
	bool m_bInit;

	std::vector<IPCReceiver*> m_vecMsgReceiver;

    static std::shared_ptr<IPCHost> inst;
};


#define BEGIN_IPCMSG_MAP(theclass)	\
	virtual bool OnIPCMessage(UINT16 uiMsgID, LPBYTE lpData, UINT32 dwSize, std::shared_ptr<BYTE> &lpReturnData, UINT32 *lpdwReturnSize) override	\
{	\
	switch(uiMsgID)	\
{

#define IPCMSG_HANDLER(msg, func)	\
		case msg:	\
		return func(lpData, dwSize);

#define IPCSYNCMSG_HANDLER(msg, func)	\
		case msg:	\
		return func(lpData, dwSize, lpReturnData, lpdwReturnSize);

#define END_IPCMSG_MAP()	\
}	\
	return false;	\
}

class IPCReceiver
{
friend class IPCHost;
protected:

	//返回TRUE表明消息已经被Receiver处理，不会再递交给下一个注册了的Receiver；FALSE则会递交给下一个注册了的Receiver
	virtual bool OnIPCMessage(UINT16 uiMsgID, LPBYTE lpData, UINT32 dwSize, std::shared_ptr<BYTE> &lpReturnData, UINT32 *lpdwReturnSize) = 0;
    virtual void OnIPCDisconnect() = 0;
};

#endif