#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Time.h"

#include <unordered_map>
#include <mutex>

// Athena stats
namespace Athena
{
	class StatsGroup;

	class CycleStat
	{
	public:
		struct Snapshot
		{
			std::string_view Name;
			Time Value;
		};

	public:
		CycleStat(std::string_view name, StatsGroup* statsGroup);

		void Reset()
		{
			m_Value = 0.f;
		}

		void Increase(Time IncreaseValue)
		{
			m_Value += IncreaseValue;
		}

		Snapshot GetSnapshot() const
		{
			return Snapshot(m_Name, m_Value);
		}

	private:
		std::string_view m_Name;
		Time m_Value;
	};

	class ScopedCycleStatProxy
	{
	public:
		ScopedCycleStatProxy(CycleStat* cycleStat)
			: m_StatRef(cycleStat)
		{

		}

		~ScopedCycleStatProxy()
		{
			m_StatRef->Increase(m_Timer.ElapsedTime());
		}

	private:
		CycleStat* m_StatRef;
		Timer m_Timer;
	};


	class CounterStat
	{
	public:
		struct Snapshot
		{
			std::string_view Name;
			int64 Value;
		};

	public:
		CounterStat(std::string_view name, StatsGroup* statsGroup);

		void Set(int64 newValue)
		{
			m_Value = newValue;
		}

		void Increment()
		{
			++m_Value;
		}

		void Decrement()
		{
			--m_Value;
		}

		void IncrementBy(int64 incrementValue)
		{
			m_Value += incrementValue;
		}

		void DecrementBy(int64 decrementValue)
		{
			m_Value -= decrementValue;
		}

		Snapshot GetSnapshot() const
		{
			return Snapshot(m_Name, m_Value);
		}

	private:
		std::string_view m_Name;
		int64 m_Value;
	};

	enum StatsThread
	{
		GameThread = 0,
		RenderThread,
		Num
	};

	class StatsGroup
	{
	public:
		struct Snapshot
		{
			std::vector<CycleStat::Snapshot> CycleStats;
			std::vector<CounterStat::Snapshot> CounterStats;
		};

	public:
		StatsGroup(std::string_view name, StatsThread thread);

		void RegisterStat(CycleStat* stat)
		{
			m_CycleStats.push_back(stat);
		}

		void RegisterStat(CounterStat* stat)
		{
			m_CounterStats.push_back(stat);
		}

		void HeartBeat()
		{
			UpdateSnapshot();
			ResetStats();
		}

		void UpdateSnapshot()
		{
			std::unique_lock<std::mutex> lock(m_SnapshotMutex);

			m_Snapshot.CycleStats.resize(m_CycleStats.size());
			m_Snapshot.CounterStats.resize(m_CounterStats.size());

			for (uint32 i = 0; i < m_CycleStats.size(); ++i)
			{
				m_Snapshot.CycleStats[i] = m_CycleStats[i]->GetSnapshot();
			}
			std::sort(m_Snapshot.CycleStats.begin(), m_Snapshot.CycleStats.end(), 
				[](const auto& left, const auto& right) { return left.Value > right.Value; });

			for (uint32 i = 0; i < m_CounterStats.size(); ++i)
			{
				m_Snapshot.CounterStats[i] = m_CounterStats[i]->GetSnapshot();
			}
			std::sort(m_Snapshot.CounterStats.begin(), m_Snapshot.CounterStats.end(), 
				[](const auto& left, const auto& right) { return left.Value > right.Value; });
		}

		void ResetStats()
		{
			for (auto& stat : m_CycleStats)
			{
				stat->Reset();
			}

			// do not reset counter stats
		}

		Snapshot GetSnapshot() const
		{
			std::unique_lock<std::mutex> lock(m_SnapshotMutex);
			return m_Snapshot;
		}

		std::string_view GetName() const
		{
			return m_Name;
		}

		StatsThread GetThread() const
		{
			return m_StatsThread;
		}

	private:
		std::string_view m_Name;
		StatsThread m_StatsThread;
		std::vector<CycleStat*> m_CycleStats;
		std::vector<CounterStat*> m_CounterStats;

		mutable std::mutex m_SnapshotMutex;
		Snapshot m_Snapshot;
	};


	class ATHENA_API StatsSystem
	{
	public:
		static StatsSystem& Get()
		{
			static StatsSystem s_Instance;
			return s_Instance;
		}

		StatsSystem()
		{
			for (auto& frameNumber : ThreadFrameNumber)
			{
				frameNumber = 0;
			}
		}

		void RegisterStatsGroup(StatsGroup* statsGroup)
		{
			m_StatsGroups.insert({ statsGroup->GetName(),statsGroup });
		}

		void ThreadHeartBeat(StatsThread thread)
		{
			ThreadFrameNumber[thread]++;

			for (const auto& [name, group] : m_StatsGroups)
			{
				if (group->GetThread() == thread)
				{
					if ((ThreadFrameNumber[thread] % HeartBeatFrameRate == 0))
						group->HeartBeat();
					else
						group->ResetStats();
				}
			}
		}

		StatsGroup::Snapshot GetStatsGroupSnapshot(std::string_view name)
		{
			ensuref(m_StatsGroups.contains(name));

			if (m_StatsGroups.contains(name))
				return m_StatsGroups.at(name)->GetSnapshot();

			return {};
		}

		const auto& GetAllStatsGroups() const
		{
			return m_StatsGroups;
		}

	private:
		std::unordered_map<std::string_view, StatsGroup*> m_StatsGroups;
		uint64 ThreadFrameNumber[StatsThread::Num];
		const uint64 HeartBeatFrameRate = 10;
	};


	inline StatsGroup::StatsGroup(std::string_view name, StatsThread thread)
		: m_Name(name), m_StatsThread(thread)
	{
		StatsSystem::Get().RegisterStatsGroup(this);
	}

	inline CycleStat::CycleStat(std::string_view name, StatsGroup* statsGroup)
		: m_Name(name), m_Value(0.f)
	{
		statsGroup->RegisterStat(this);
	}

	inline CounterStat::CounterStat(std::string_view name, StatsGroup* statsGroup)
		: m_Name(name), m_Value(0)
	{
		statsGroup->RegisterStat(this);
	}

#if ATN_ENABLE_STATS
	#define DEFINE_STATS_GROUP(Label, StatsGroupName, StatsThreadID) ATHENA_API StatsGroup StatsGroupName(Label, StatsThreadID)
	#define EXTERN_STATS_GROUP(StatsGroupName) extern ATHENA_API StatsGroup StatsGroupName

	#define DEFINE_CYCLE_STAT(Label, StatName, StatsGroupName) ATHENA_API CycleStat StatName(Label, &StatsGroupName)
	#define EXTERN_CYCLE_STAT(StatName) extern ATHENA_API CycleStat StatName

	#define DEFINE_COUNTER_STAT(Label, StatName, StatsGroupName) ATHENA_API CounterStat StatName(Label, &StatsGroupName)
	#define EXTERN_COUNTER_STAT(StatName) extern ATHENA_API CounterStat StatName


	#define SCOPE_CYCLE_STAT(StatName) ScopedCycleStatProxy StatName##_Proxy(&StatName)

	#define SET_COUNTER_STAT(StatName, Value) StatName.Set(Value)
	#define INC_COUNTER_STAT(StatName) StatName.Increment()
	#define DEC_COUNTER_STAT(StatName) StatName.Decrement()
	#define INC_COUNTER_STAT_BY(StatName, Value) StatName.IncrementBy(Value)
	#define DEC_COUNTER_STAT_BY(StatName, Value) StatName.DecrementBy(Value)

	#define STATS_THREAD_HEARTBEAT(StatsThreadID) StatsSystem::Get().ThreadHeartBeat(StatsThreadID)
#else
	#define DEFINE_STATS_GROUP(Label, StatsGroupName) 
	#define EXTERN_STATS_GROUP(Label, StatsGroupName)

	#define DEFINE_CYCLE_STAT(Label, StatName, StatsGroupName)
	#define EXTERN_CYCLE_STAT(Label, StatName, StatsGroupName)

	#define DEFINE_COUNTER_STAT(Label, StatName, StatsGroupName)
	#define EXTERN_COUNTER_STAT(Label, StatName, StatsGroupName)


	#define SCOPE_CYCLE_STAT(StatName)

	#define SET_COUNTER_STAT(StatName, Value)
	#define INC_COUNTER_STAT(StatName)
	#define DEC_COUNTER_STAT(StatName)
	#define INC_COUNTER_STAT_BY(StatName, Value)
	#define DEC_COUNTER_STAT_BY(StatName, Value)

	#define STATS_GROUP_END_FRAME(StatsGroupName)
#endif
}
