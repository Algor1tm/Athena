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

		void Suspend();
		void Resume();

		uint64 GetID() const { return m_ID; }
		const String& GetName() const { return m_Name; }

		static uint64 GetCurrentThreadID();
		static bool IsGameThread() { return GetCurrentThreadID() == s_GameThreadID; }
		static bool IsRenderThread() { return GetCurrentThreadID() == s_RenderThreadID; }

		static void SleepFor(Time time);
		static void YieldExecution();

	protected:
		static inline uint64 s_RenderThreadID = 0;
		static inline uint64 s_GameThreadID = 0;

	private:
		static unsigned long ThreadProcedure(void* args);

	private:
		String m_Name;
		std::function<void()> m_Function;
		uint64 m_ID = 0;

		void* m_Handle = nullptr;
	};
}
