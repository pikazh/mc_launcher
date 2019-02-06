#pragma once

#include <memory>
#include <stdint.h>

#include "closure.h"

class SimpleMemoryPool;

class Pickle
{
    friend class PickleIterator;
public:
    Pickle(size_t init_size = 1024);
    Pickle(std::shared_ptr<SimpleMemoryPool> pool, size_t init_size = 1024);
    virtual ~Pickle();

    void reset_content();

    //attach 的buffer大小不能小于平台的指针大小字节;地址必须内存对齐
    template<typename Dx>
    bool attach_to_byte_array(char* buf, size_t len, Dx dx);

    char* detach_from_byte_array(size_t &buffer_len, size_t &content_len);
    char* get_raw_byte_array_ptr(size_t &buffer_len, size_t &content_len);

    template <typename Cls> inline void write_integer(Cls value)
    {
        write_char_array_no_len((const char*)&value, sizeof(value));
    }
    void write_char_array(const char* array, size_t len);
    void write_char_array_no_len(const char* array, size_t len);
protected:
    inline void init(size_t init_size = 1024)
    {
        all_buffer_len_ = init_size;
        buffer_ = alloc_buffer(all_buffer_len_);
        memset(buffer_, 0, all_buffer_len_);
        header_ = (header*)buffer_;
        header_->next_write_offset = sizeof(header);
        deleter_ = make_deleter(buffer_);
    }

    struct header
    {
        size_t next_write_offset;
    };
    inline char* alloc_buffer(size_t sz);
    inline std::shared_ptr<Closure> make_deleter(char* buf);
    void expand_buffer(size_t len);
    header* get_header();
private:
    std::shared_ptr<SimpleMemoryPool> memory_pool_;
    std::shared_ptr<Closure> deleter_;
    char* buffer_ = nullptr;
    size_t all_buffer_len_ = 0;
    header *header_ = nullptr;
};

template<typename Dx>
bool Pickle::attach_to_byte_array(char* buf, size_t len, Dx dx)
{
    size_t align_unit = sizeof(void*);
    size_t addr = (size_t)buf;
    if ((addr & (align_unit - 1)) != 0)
        return false;
    if (len < sizeof(void*))
    {
        return false;
    }

    if (buffer_)
    {
        deleter_->run();
        deleter_.reset();
        buffer_ = nullptr;
    }

    buffer_ = buf;
    all_buffer_len_ = len;
    header_ = (header*)buffer_;
    deleter_ = make_closure(dx, buffer_);

    return true;
}

class PickleIterator
{
public:
    PickleIterator(Pickle &pickle);
    virtual ~PickleIterator() = default;

public:
    template <typename Cls> inline Cls read_pod()
    {
        return *(Cls*)read_char_array_no_len(sizeof(Cls));
    }

    char* read_char_array(size_t &len);
    char* read_char_array_no_len(size_t len);

private:
#ifdef _DEBUG
    Pickle &pickle_;
    Pickle::header *header_ = nullptr;
#endif
    char *buffer_ = nullptr;
    size_t offset_ = 0;
};

