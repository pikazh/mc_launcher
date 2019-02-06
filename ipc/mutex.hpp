#ifndef _MUTEX_HPP_
#define _MUTEX_HPP_

#include <string>

#ifdef _WINDOWS
#include <windows.h>
#elif defined _UNIX
#include <pthread.h>
#endif

namespace System
{

class Mutex
{
public:
    Mutex() : m_bCreate(false) {}
    Mutex(const char* name) : m_bCreate(false)
    {
        Create(name);
    }
    ~Mutex()
    {
        Release();
    }
public:
    bool Create(const char* name)
    {
        if (m_bCreate) return false;
        m_hMutex = ::CreateMutexA(NULL, FALSE, name);
        if (m_hMutex)
        {
            m_bCreate = true;
            return true;
        }
        return false;
    }
    bool Release()
    {
        if (m_bCreate)
        {
            m_bCreate = false;
            ::CloseHandle(m_hMutex);
            return true;
        }
        return false;
    }
    bool Open(const char* name)
    {
        if (m_bCreate) return false;
        m_hMutex = ::OpenMutexA(MUTEX_ALL_ACCESS, FALSE, name);
        if (m_hMutex)
        {
            m_bCreate = true;
            return true;
        }
        return false;
    }
    int Lock()
    {
        return (int)::WaitForSingleObject(m_hMutex, INFINITE);
    }

    int TryLock(unsigned long ms)
    {
        return (int)::WaitForSingleObject(m_hMutex, ms);
    }

    int Unlock()
    {
        return (int)::ReleaseMutex(m_hMutex);
    }
protected:
    HANDLE  m_hMutex;
    bool m_bCreate;
protected:
    Mutex(const Mutex& rhs) {}
    const Mutex& operator=(const Mutex& rhs)
    {
        return *this;
    }
};

class MutexGuard
{
public:
    MutexGuard(Mutex& mutex) : m_mutex(mutex)
    {
    }
    ~MutexGuard()
    {
        m_mutex.Unlock();
    }
protected:
    Mutex& m_mutex;
};

}//namespace System

#endif