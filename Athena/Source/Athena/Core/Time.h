#pragma once

#include <chrono>


namespace Athena
{
	class Time
	{
	public:
		using duration_type = std::chrono::duration<float, std::micro>;

	public:
		Time(float seconds = 0)
			: m_Time(std::chrono::duration<float>(seconds)) {}

		template <typename Rep, typename Period>
		Time(std::chrono::duration<Rep, Period> duration)
			: m_Time(std::chrono::duration_cast<duration_type>(duration)) {}

		inline operator float() const
		{
			return m_Time.count();
		}

		inline float AsSeconds() const
		{
			return m_Time.count() * 0.000001f;
		}

		inline float AsMilliseconds() const
		{
			return m_Time.count() * 0.001f;
		}

		inline float AsMicroseconds() const
		{
			return m_Time.count();
		}
		
		inline Time& operator+=(Time other)
		{
			m_Time += other.m_Time;
			return *this;;
		}

		inline Time& operator-=(Time other)
		{
			m_Time -= other.m_Time;
			return *this;;
		}

		inline Time operator+(Time other) const
		{
			return Time(m_Time + other.m_Time);
		}

		inline Time operator-(Time other) const
		{
			return Time(m_Time - other.m_Time);
		}

		inline bool operator==(Time other) const
		{
			return m_Time == other.m_Time;
		}

		inline bool operator!=(Time other) const
		{
			return m_Time != other.m_Time;
		}

		inline bool operator>(Time other) const
		{
			return m_Time > other.m_Time;
		}

		inline bool operator<(Time other) const
		{
			return m_Time < other.m_Time;
		}

		inline bool operator>=(Time other) const
		{
			return m_Time >= other.m_Time;
		}

		inline bool operator<=(Time other) const
		{
			return m_Time <= other.m_Time;
		}

	public:
		static inline Time Seconds(float seconds)
		{
			return Time(seconds);
		}

		static inline Time Milliseconds(float milliseconds)
		{
			return Time(std::chrono::duration<float, std::milli>(milliseconds));
		}

		static inline Time Microseconds(float microseconds)
		{
			return Time(std::chrono::duration<float, std::micro>(microseconds));
		}

	private:
		duration_type m_Time;
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

		inline Time ElapsedTime()
		{
			auto end = std::chrono::high_resolution_clock::now();
			return Time(end - m_Start);
		}

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
	};

}
