#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Thread.h"

#include <Windows.h>


namespace Athena
{
	unsigned long Thread::ThreadProcedure(void* args)
	{
		Thread* thread = static_cast<Thread*>(args);

		ATN_PROFILE_THREAD(thread->GetName().data());
		thread->m_Function();

		return 0;
	}

	Thread::Thread(const String& name, std::function<void()>&& func)
	{
		m_Name = name;
		m_Function = std::move(func);

		DWORD threadID = 0;

		m_Handle = CreateThread(
			nullptr,                
			0,                      
			&ThreadProcedure,
			this,
			CREATE_SUSPENDED,       
			&threadID
		);

		ATN_CORE_ASSERT(m_Handle);

		if (m_Handle)
		{
			m_ID = threadID;
			SetThreadDescription(m_Handle, FilePath(name).wstring().data());
		}
	}

	Thread::~Thread()
	{
		if (m_Handle)
			CloseHandle(m_Handle);
	}

	void Thread::Start()
	{
		Resume();
	}

	void Thread::Pause()
	{
		if (!m_Handle)
			return;

		DWORD result = SuspendThread(m_Handle);
		ATN_CORE_ASSERT(result != (DWORD)-1);
	}

	void Thread::Resume()
	{
		if (!m_Handle)
			return;

		DWORD result = ResumeThread(m_Handle);
		ATN_CORE_ASSERT(result != (DWORD)-1);
	}

	void Thread::Join()
	{
		if (!m_Handle)
			return;

		DWORD result = WaitForSingleObject(m_Handle, INFINITE);
		ATN_CORE_ASSERT(result == WAIT_OBJECT_0);
	}

	uint64 Thread::GetCurrentThreadID()
	{
		DWORD id = GetCurrentThreadId();
		return id;
	}

	void Thread::CurrentThreadSleep(Time time)
	{
		DWORD millis = std::round(time.AsMilliseconds());
		Sleep(millis);
	}
}
