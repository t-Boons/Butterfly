#pragma once
#include <functional>
#include <vector>

namespace Butterfly
{
	struct EventHandle
	{
	private:
		template <typename InType>
		friend class EventDispatcher;

		EventHandle() = default;

		uint64_t DispatcherID = 0;
		uint64_t EventID = 0;
	};

	template <typename InType = void>
	class EventDispatcher
	{
	public:
		using Func = std::function<void(const InType&)>;

		EventDispatcher()
			: m_dispatcherID(Utils::RandomUint64())
		{
		}

		EventHandle Subscribe(const Func& func)
		{
			EventHandle handle;
			handle.DispatcherID = m_dispatcherID;
			handle.EventID = m_nextEventID++;

			m_funcs.push_back({handle, func});

			return handle;
		}

		void Unsubscribe(const EventHandle& handle)
		{
			if (handle.DispatcherID != m_dispatcherID)
			{
				return;
			}

			auto it = std::find_if(m_funcs.begin(), m_funcs.end(), [&](const auto& entry)
				{
					return entry.Handle.EventID == handle.EventID;
				});

			if (it != m_funcs.end())
			{
				m_funcs.erase(it);
			}
		}

		void Broadcast(const InType& val)
		{
			for (auto& event : m_funcs)
			{
				event.Func(val);
			}
		}

	private:
		struct Event
		{
			EventHandle Handle;
			Func Func;
		};

		uint64_t m_dispatcherID;
		uint64_t m_nextEventID = 1;
		std::vector<Event> m_funcs;
	};

	template <>
	class EventDispatcher<void>
	{
	public:
		using Func = std::function<void()>;

		EventHandle Subscribe(const Func& func)
		{
			EventHandle handle;
			handle.DispatcherID = m_dispatcherID;
			handle.EventID = m_nextEventID++;

			m_funcs.push_back({ handle, func });

			return handle;
		}

		void Unsubscribe(const EventHandle& handle)
		{
			if (handle.DispatcherID != m_dispatcherID)
				return;

			auto it = std::find_if(m_funcs.begin(), m_funcs.end(),
				[&](const auto& entry)
				{
					return entry.Handle.EventID == handle.EventID;
				});

			if (it != m_funcs.end())
				m_funcs.erase(it);
		}

		void Broadcast()
		{
			for (auto& event : m_funcs)
				event.Func();
		}

	private:
		struct Event
		{
			EventHandle Handle;
			Func Func;
		};

		uint64_t m_dispatcherID = Utils::RandomUint64();
		uint64_t m_nextEventID = 1;
		std::vector<Event> m_funcs;
	};
}