#ifndef _CRITICAL_SECTION_HPP_
#define _CRITICAL_SECTION_HPP_

#include <windows.h>

namespace System
{

	class CriticalSection
	{
	public:
		CriticalSection()
		{
			::InitializeCriticalSection(&m_cs);
		}
		~CriticalSection()
		{
			::DeleteCriticalSection(&m_cs);
		}
	public:
		inline void Lock()
		{
			::EnterCriticalSection(&m_cs);
		}
		inline void Unlock()
		{
			::LeaveCriticalSection(&m_cs);
		}
	protected:
		CRITICAL_SECTION m_cs;

	protected:
		CriticalSection(const CriticalSection& rhs) {}
		CriticalSection& operator=(const CriticalSection& rhs)
		{
			return *this;
		}
	};

	class CriticalSectionGuard
	{
	public:
		CriticalSectionGuard(CriticalSection& theCS) : m_cs(theCS) {}
		~CriticalSectionGuard()
		{
			m_cs.Unlock();
		}
	protected:
		CriticalSection&  m_cs;
	protected:
		CriticalSectionGuard(const CriticalSectionGuard& rhs) : m_cs(*(new CriticalSection())) {}
		const CriticalSectionGuard& operator=(const CriticalSectionGuard& rhs)
		{
			return *this;
		}
	};

}//namespace System

#endif