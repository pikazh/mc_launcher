#include "memory_helper.h"

SimpleMemoryPool* GetMemoryPool()
{
    static SimpleMemoryPool pool;
    return &pool;
}

SimpleObjectPool<SharePacket>* GetSharePacketPool()
{
    static SimpleObjectPool<SharePacket> pool;
    return &pool;
}
