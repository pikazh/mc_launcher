#ifndef __IPC_LIB_H__
#define __IPC_LIB_H__

#ifdef _IPC_INTERNAL
#define IPC_API __declspec(dllexport)
#else
#define IPC_API __declspec(dllimport)
#endif

#include <windows.h>
#define IPCHANDLE(name) struct name##__ { int unused; }; typedef struct name##__ *name
#define IPCHANDLE_INTERNAL(name) union name { UINT32 handle; void* pvoid; }


#ifndef _IPC_INTERNAL

IPCHANDLE(HCHANNEL);
IPCHANDLE(HSHAREMEM);

#else

IPCHANDLE_INTERNAL(HCHANNEL);
IPCHANDLE_INTERNAL(HSHAREMEM);

#endif

#define IPC_NAME_MAX 32
#define IPC_CHANNEL_MAX 80

#define IPC_PORT 1
#define IPC_APP_NAME ("craftbox")

extern "C"
{

#pragma pack(push)
#pragma pack(1)

typedef struct tagChannelAddr
{
    char Name[IPC_NAME_MAX];
    UINT16 Port;
} ChannelAddr, *PChannelAddr;

enum SharePacketType
{
    SHARE_PCK_NORMAL = 0,
    SHARE_PCK_REQUEST,
	SHARE_PCK_REQUEST_SYNC,
    SHARE_PCK_RESPONSE
};

enum ShareChannelEvent
{
    CHANNEL_EVENT_RECV = 0,
    CHANNEL_EVENT_DISCONNECT,
    CHANNEL_EVENT_CLOSE,
    CHANNEL_EVENT_TIMEOUT,
    CHANNEL_EVENT_ERROR,
};

typedef struct tagSharePacketHeader
{
	UINT16 uiProtocolID;
    char DestName[IPC_NAME_MAX];
    char SrcName[IPC_NAME_MAX];
    UINT16 Port;
    UINT8 PacketType;
    UINT32 Seq;
} SharePacketHeader, *PSharePacketHeader;

typedef struct tagSharePacketData
{
    UINT32 BodyBufferLen;
    BYTE* BodyBuffer;
} SharePacketData, *PSharePacketData;

typedef struct tagSharePacket
{
    SharePacketHeader Header;
    SharePacketData Data;
} SharePacket, *PSharePacket;


IPC_API bool IPCStartup(const char* pszName);
IPC_API bool IPCCleanup();
IPC_API bool IPCTryCleanup();
IPC_API bool IPCIsActive();
IPC_API const char* IPCGetName();
IPC_API bool IPCCheckActive(const char* pszName);

IPC_API HCHANNEL ChannelCreate(UINT16 nPort);
IPC_API bool ChannelDestroy(HCHANNEL hChannel);
IPC_API bool ChannelClose(HCHANNEL hChannel);
IPC_API int ChannelSend(HCHANNEL hChannel, const char* pszName, UINT16 uiProtocolID, unsigned char* pBuffer, UINT32 len);
IPC_API int ChannelSendRequest(HCHANNEL hChannel, const char* pszName, UINT16 uiProtocolID, unsigned char* pBuffer, UINT32 len);
IPC_API int ChannelSendRequestSync(HCHANNEL hChannel, const char* pszName, UINT16 uiProtocolID, unsigned char* pBuffer, UINT32 len, PSharePacket* pResponsePacket, UINT nTimeout);
IPC_API int ChannelSendResponse(HCHANNEL hChannel, const char* pszName, UINT16 uiProtocolID, unsigned char* pBuffer, UINT32 len, UINT32 nSeq);
IPC_API int ChannelWaitForEvent(HCHANNEL hChannel, int nTimeout);
IPC_API PSharePacket ChannelGetRecvPacket(HCHANNEL hChannel);
IPC_API bool ChannelGetDisconnect(HCHANNEL hChannel, PChannelAddr pChannelAddr);

IPC_API PSharePacket CreateSharePacket();
IPC_API bool DestroySharePacket(PSharePacket pPacket);

IPC_API HSHAREMEM ShareMemoryCreate(const char* pszName, UINT32 nSize);
IPC_API HSHAREMEM ShareMemoryOpen(const char* pszName);
IPC_API bool ShareMemoryDestroy(HSHAREMEM hShareMem);
IPC_API bool ShareMemoryConnect(HSHAREMEM hShareMem, const char* pszName);
IPC_API bool ShareMemoryDisconnect(HSHAREMEM hShareMem);
IPC_API const char* ShareMemoryGetName(HSHAREMEM hShareMem);
IPC_API bool ShareMemoryIsValid(HSHAREMEM hShareMem);
IPC_API UINT32 ShareMemoryGetSize(HSHAREMEM hShareMem);
IPC_API unsigned char* ShareMemoryGetData(HSHAREMEM hShareMem);

#pragma pack(pop)

}

#endif