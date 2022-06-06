#pragma once


namespace Athena
{
	class Time
	{
	public:
		Time(float milliSeconds = 0)
			: m_Time(milliSeconds) {}

		inline operator float() const
		{
			return m_Time;
		}

		inline float AsSeconds() const
		{
			return m_Time * 0.001f;
		}

		inline float AsMilliseconds() const
		{
			return m_Time;
		}

		inline Time operator-(Time other) const
		{
			return Time(m_Time - other.m_Time);
		}

		static inline Time Seconds(float seconds)
		{
			return Time(seconds * 1000);
		}

		static inline Time Milliseconds(float milliseconds)
		{
			return Time(milliseconds);
		}

	private:
		// Milliseconds
		float m_Time;
	};


	class Timer
	{
	public:
		Timer()
		{
			Reset();
		}

		inline void Reset()
		{
			m_Start = std::chrono::high_resolution_clock::now();
		}

		inline Time GetElapsedTime()
		{
			return Time::Milliseconds((float)std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::high_resolution_clock::now() - m_Start).count());
		}

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
	};
}
