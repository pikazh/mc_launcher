#include "simple_memory_pool.h"
#include "simple_object_pool.h"

SimpleMemoryPool::~SimpleMemoryPool()
{
	std::lock_guard<decltype(lock_)> l(lock_);

    for (auto it = pools_.begin(); it != pools_.end(); it++)
    {
        delete it->second;
    }
}

void* SimpleMemoryPool::alloca(size_t size)
{
	std::lock_guard<decltype(lock_)> l(lock_);

    size = get_arrange_size(size);
    auto it = pools_.find(size);
    if (it != pools_.end())
    {
        void* alloca_block = it->second->popObject();
        alloca_blocks_2_pool_[alloca_block] = size;
        return alloca_block;
    }
    else
    {
        auto obj = new SimpleObjectPool<void*>(size);
        pools_[size] = obj;

        void* alloca_block = obj->popObject();
        alloca_blocks_2_pool_[alloca_block] = size;
        return alloca_block;
    }
}

void SimpleMemoryPool::free(void* ptr)
{
	std::lock_guard<decltype(lock_)> l(lock_);

    auto it = alloca_blocks_2_pool_.find(ptr);
    assert(it != alloca_blocks_2_pool_.end());
    if (it != alloca_blocks_2_pool_.end())
    {
        auto iter = pools_.find(it->second);
        iter->second->pushObject(ptr);
    }
}

size_t SimpleMemoryPool::get_arrange_size(size_t sz)
{
    size_t new_size = sizeof(int*);
    if (sz <= new_size)
        return new_size;

    new_size = 1;
    size_t old_size = sz;
    sz >>= 1;
    while (sz != 0)
    {
        sz >>= 1;
        new_size <<= 1;
    }

    if (new_size != old_size)
        new_size <<= 1;

    return new_size;
}
