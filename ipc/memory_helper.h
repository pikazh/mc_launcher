#pragma once

#include "base/simple_memory_pool.h"
#include "base/simple_object_pool.h"
#include "ipc.h"

#define MemoryPool (GetMemoryPool())
#define SharePacketPool (GetSharePacketPool())

SimpleMemoryPool* GetMemoryPool();

SimpleObjectPool<SharePacket>* GetSharePacketPool();