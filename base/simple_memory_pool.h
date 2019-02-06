#pragma once

#include "simple_object_pool.h"

#include <map>
#include <mutex>

class SimpleMemoryPool
{
public:
    SimpleMemoryPool() = default;
    virtual ~SimpleMemoryPool();

public:
    void* alloca(size_t size);
    void free(void* ptr);

protected:
    size_t get_arrange_size(size_t sz);
private:
    std::map<size_t, SimpleObjectPool<void*>*> pools_;
    std::map<void*, size_t> alloca_blocks_2_pool_;
	std::recursive_mutex lock_;
};