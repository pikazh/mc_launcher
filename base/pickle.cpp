#pragma once

#include "pickle.h"
#include "simple_memory_pool.h"

#include <assert.h>

Pickle::Pickle(size_t init_size)
{
    init(init_size);
}

Pickle::Pickle(std::shared_ptr<SimpleMemoryPool> pool, size_t init_size)
{
    memory_pool_ = pool;
    init(init_size);
}

Pickle::~Pickle()
{
    if (!!deleter_)
        deleter_->run();
}

 
void Pickle::reset_content()
{
    if (buffer_ != nullptr)
    {
        header_->next_write_offset = sizeof(header_);
    }
}

char* Pickle::detach_from_byte_array(size_t &buffer_len, size_t &content_len)
{
    if (buffer_ == nullptr)
    {
        buffer_len = 0;
        content_len = 0;
        return nullptr;
    }

    buffer_len = all_buffer_len_;
    content_len = header_->next_write_offset;
    all_buffer_len_ = 0;
    header_ = nullptr;
    char* ret = buffer_;
    buffer_ = nullptr;
    deleter_.reset();
    return ret;
}

char* Pickle::get_raw_byte_array_ptr(size_t &buffer_len, size_t &content_len)
{
    if (buffer_ == nullptr)
    {
        buffer_len = 0;
        content_len = 0;
        return nullptr;
    }

    buffer_len = all_buffer_len_;
    content_len = header_->next_write_offset;
    return buffer_;
}

void Pickle::write_char_array(const char* array, size_t len)
{
    while (header_->next_write_offset + len + sizeof(len) > all_buffer_len_)
    {
        expand_buffer(all_buffer_len_ << 1);
    }

    *(size_t*)(buffer_ + header_->next_write_offset) = len;
    header_->next_write_offset += sizeof(len);
    memcpy((char*)buffer_ + header_->next_write_offset, array, len);
    size_t offset = header_->next_write_offset + len;
    size_t size_unit = sizeof(void*);
    if ((offset & (size_unit - 1)) != 0)
    {
        header_->next_write_offset = (offset & (~(size_unit - 1))) + size_unit;
    }
    else
        header_->next_write_offset = offset;
}

void Pickle::write_char_array_no_len(const char* array, size_t len)
{
    while (header_->next_write_offset + len > all_buffer_len_)
    {
        expand_buffer(all_buffer_len_ << 1);
    }

    memcpy(buffer_ + header_->next_write_offset, array, len);
    size_t offset = header_->next_write_offset + len;
    size_t size_unit = sizeof(void*);
    if ((offset & (size_unit - 1)) != 0)
    {
        header_->next_write_offset = (offset & (~(size_unit - 1))) + size_unit;
    }
    else
        header_->next_write_offset = offset;
}

char* Pickle::alloc_buffer(size_t sz)
{
    if (!!memory_pool_)
    {
        return (char*)memory_pool_->alloca(sz);
    }
    else
    {
        return (char*)malloc(sz);
    }
}

std::shared_ptr<Closure> Pickle::make_deleter(char *buf)
{
    if (!!memory_pool_)
        return make_closure(&SimpleMemoryPool::free, memory_pool_, buf);
    else
        return make_closure(free, buf);
}

void Pickle::expand_buffer(size_t len)
{
    all_buffer_len_ = len;

    char *buffer = alloc_buffer(all_buffer_len_);
    memcpy(buffer, buffer_, header_->next_write_offset);
    deleter_->run();
    deleter_ = make_deleter(buffer);
    buffer_ = buffer;
    header_ = (header*)buffer_;
}

Pickle::header* Pickle::get_header()
{
    return header_;
}

PickleIterator::PickleIterator(Pickle &pickle)
#ifdef _DEBUG
    : pickle_(pickle)
#endif
{
#ifdef _DEBUG
    header_ = pickle_.get_header();
#endif
    size_t sz = 0;
    size_t content_len = 0;
    buffer_ = pickle.get_raw_byte_array_ptr(sz, content_len);
    offset_ = sizeof(Pickle::header);
}

char* PickleIterator::read_char_array(size_t &len)
{
    len = read_pod<size_t>();
    return read_char_array_no_len(len);
}

char* PickleIterator::read_char_array_no_len(size_t len)
{
#ifdef _DEBUG
    assert(offset_ + len <= header_->next_write_offset);
#endif
    char *ret = buffer_ + offset_;
    offset_ += len;
    size_t size_unit = sizeof(void*);
    if ((offset_ & (size_unit - 1)) != 0)
    {
        offset_ = (offset_ & (~(size_unit - 1))) + size_unit;
    }

    return ret;
}
