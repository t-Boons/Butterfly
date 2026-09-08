#include "Core/JobSystem.hpp"

namespace Butterfly
{
	bool JobHandle::IsComplete() const
	{
		return m_complete.load();
	}

	void JobHandle::Wait() const
	{
		std::unique_lock lock(m_mutex);

		m_jobComplete.wait(lock, [this]
			{
				return m_complete.load();
			});
	}


	JobSystem::JobSystem()
	{
		m_threadCount = std::thread::hardware_concurrency();
		m_shouldStop = false;

		// Leave one thread available for the main thread.
		m_threadCount--;

		if (m_threadCount == 0)
		{
			m_threadCount = 1;
		}

		m_workers.reserve(m_threadCount);

		for (uint32_t i = 0; i < m_threadCount; i++)
		{
			m_workers.emplace_back(&JobSystem::Worker, this);
		}
	}

	JobSystem::~JobSystem()
	{
		m_shouldStop = true;

		m_jobsAvailable.notify_all();

		for (std::thread& worker : m_workers)
		{
			if (worker.joinable())
			{
				worker.join();
			}
		}
	}

	RefPtr<JobHandle> JobSystem::Submit(const std::function<void()>& work)
	{
		RefPtr<JobHandle> handle = MakeRef<JobHandle>();

		{
			std::lock_guard lock(m_queueMutex);

			m_jobs.push({ std::move(work), handle });
		}

		m_jobsAvailable.notify_one();

		return handle;
	}

	void JobSystem::Worker()
	{
		while (true)
		{
			Job job;

			{
				std::unique_lock lock(m_queueMutex);

				m_jobsAvailable.wait(lock, [this]
					{
						return !m_jobs.empty() || m_shouldStop;
					});

				if (m_shouldStop && m_jobs.empty())
				{
					return;
				}

				job = std::move(m_jobs.front());
				m_jobs.pop();
			}

			job.Function();

			job.Handle->m_complete = true;
			job.Handle->m_jobComplete.notify_all();
		}
	}
}