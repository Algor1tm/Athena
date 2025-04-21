#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Time.h"

#include <functional>


namespace Athena
{
	class Thread
	{
	public:
		Thread(const String& name, std::function<void()>&& func);
		~Thread();

		void Start();
		void Join();

		void Pause();
		void Resume();

		uint64 GetID() const { return m_ID; }
		const String& GetName() const { return m_Name; }

		static uint64 GetCurrentThreadID();
		static void CurrentThreadSleep(Time time);

	private:
		static unsigned long ThreadProcedure(void* args);

	private:
		String m_Name;
		std::function<void()> m_Function;
		uint64 m_ID = 0;

		void* m_Handle = nullptr;
	};
}
