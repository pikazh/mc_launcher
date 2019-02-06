#pragma once

#include <assert.h>
#include <list>
#include <stdint.h>

#include <mutex>

template<typename T>
class SimpleObjectPool
{
public:
    SimpleObjectPool(uint32_t initObjNum = 50)
    {
        reserve(initObjNum);
    }

    SimpleObjectPool(const SimpleObjectPool&) = delete;
    SimpleObjectPool(SimpleObjectPool&&) = delete;

    ~SimpleObjectPool()
    {
        assert(all_num_ == nodes_.size());
        for (auto it = nodes_.begin(); it != nodes_.end(); it++)
        {
            delete (*it);
        }
    }

    inline T* popObject()
    {
        std::lock_guard<std::recursive_mutex> l(lock_);

        if (nodes_.empty())
        {
            reserve(all_num_);
        }
        T* obj = nodes_.front();
        nodes_.pop_front();
        return obj;
    }

    inline void pushObject(T* obj)
    {
		std::lock_guard<std::recursive_mutex> l(lock_);

        nodes_.push_front(obj);
    }

protected:
    void reserve(uint32_t num)
    {
        for (uint32_t i = 0; i < num; i++)
        {
            nodes_.push_front(new T());
        }
        all_num_ += num;
    }

private:
    std::list<T*> nodes_;
    uint32_t all_num_ = 0;
	std::recursive_mutex lock_;
};

template<>
class SimpleObjectPool<void*>
{
public:
    SimpleObjectPool(size_t unit_size, uint32_t initObjNum = 50)
        : unit_size_(unit_size)
    {
        reserve(initObjNum);
    }

    SimpleObjectPool(const SimpleObjectPool&) = delete;
    SimpleObjectPool(SimpleObjectPool&&) = delete;

    ~SimpleObjectPool()
    {
        assert(all_num_ == nodes_.size());
        for (auto it = nodes_.begin(); it != nodes_.end(); it++)
        {
            free (*it);
        }
    }

    inline void* popObject()
    {
        std::lock_guard<decltype(lock_)> l(lock_);

        if (nodes_.empty())
        {
            reserve(all_num_);
        }
        void* obj = nodes_.front();
        nodes_.pop_front();
        return obj;
    }

    inline void pushObject(void* obj)
    {
        std::lock_guard<decltype(lock_)> l(lock_);

        nodes_.push_back(obj);
    }

protected:
    void reserve(uint32_t num)
    {
        for (uint32_t i = 0; i < num; i++)
        {
            nodes_.push_front(malloc(unit_size_));
        }
        all_num_ += num;
    }

private:
    std::list<void*> nodes_;
    uint32_t all_num_ = 0;
	std::recursive_mutex lock_;
    const size_t unit_size_;
};