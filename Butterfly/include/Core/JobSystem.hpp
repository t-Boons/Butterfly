#pragma once
#include "Core/Common.hpp"

namespace Butterfly
{
	class JobHandle
	{
	public:
		bool IsComplete() const;
		void Wait() const;

	private:
		std::atomic<bool> m_complete = false;

		mutable std::mutex m_mutex;
		mutable std::condition_variable m_jobComplete;

		friend class JobSystem;
	};

	struct Job
	{
		std::function<void()> Function;
		RefPtr<JobHandle> Handle;
	};

	class JobSystem : public NonCopyableNonMoveable
	{
	public:
		JobSystem();
		~JobSystem();

		RefPtr<JobHandle> Submit(const std::function<void()>& work);

	private:
		void Worker();

		std::vector<std::thread> m_workers;
		std::queue<Job> m_jobs;
		uint32_t m_threadCount;
		std::mutex m_queueMutex;
		std::condition_variable m_jobsAvailable;
		bool m_shouldStop;
	};
}