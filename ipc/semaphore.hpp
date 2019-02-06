#ifndef _SEMAPHORE_HPP_
#define _SEMAPHORE_HPP_

#include <string>
#include <windows.h>

namespace System
{
	class Semaphore
	{
	public:
		Semaphore() : m_bCreate(false) {}
		Semaphore(const char* name) : m_bCreate(false)
		{
			Create(name);
		}
		~Semaphore()
		{
			Release();
		}
	public:
		bool Create(const char* name)
		{
			if (m_bCreate) return false;
			m_hSemaphore = ::CreateSemaphoreA(NULL, 0, 5, name);
			if (m_hSemaphore)
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
				if (m_hSemaphore != INVALID_HANDLE_VALUE)
				{
					::CloseHandle(m_hSemaphore);
					return true;
				}
				else
					return false;
			}
			return false;
		}

		bool Open(const char* name)
		{
			if (m_bCreate) return false;
			m_hSemaphore = ::OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, name);
			if (m_hSemaphore)
			{
				m_bCreate = true;
				return true;
			}
			return false;
		}

		int Wait()
		{
			return (int)::WaitForSingleObject(m_hSemaphore, INFINITE);
		}

		int TryLock(unsigned long ms)
		{
			return (int)::WaitForSingleObject(m_hSemaphore, ms);
		}

		bool Active()
		{
			if (::ReleaseSemaphore(m_hSemaphore, 1, NULL))
				return true;
			else
				return false;
		}

		HANDLE Handle()
		{
			return m_hSemaphore;
		}
	protected:
		bool m_bCreate;
		HANDLE m_hSemaphore;

	protected:
		Semaphore(const Semaphore& rhs) {}
		Semaphore& operator= (const Semaphore& rhs)
		{
			return *this;
		}
	};

}//namespace System

#endif
