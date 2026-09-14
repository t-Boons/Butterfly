#pragma once
#include <functional>
#include <vector>

namespace Butterfly
{
	template<typename T>
	class EventDispatcher;

	template<typename T = void>
	class EventReceiver
	{
	public:
		~EventReceiver()
		{
			Unsubscribe();
		}

		using Func = std::function<void(const T&)>;

		void Subscribe(EventDispatcher<T>& dispatcher, const Func& func);

		void Unsubscribe();


	private:
		friend class EventDispatcher<T>;
		void InvalidateDispatcher();

		EventDispatcher<T>* m_dispatcher = nullptr;
		bool m_subscribed = false;
	};

	template<>
	class EventReceiver<void>
	{
	public:
		~EventReceiver()
		{
			Unsubscribe();
		}

		using Func = std::function<void()>;

		void Subscribe(EventDispatcher<void>& dispatcher, const Func& func);

		void Unsubscribe();

	private:
		friend class EventDispatcher<void>;
		void InvalidateDispatcher();

		EventDispatcher<void>* m_dispatcher = nullptr;
		bool m_subscribed = false;
	};

	template <typename T = void>
	class EventDispatcher
	{
	public:
		using Func = std::function<void(const T&)>;

		~EventDispatcher()
		{
			Clear();
		}

		void Broadcast(const T& val);
		void Clear();

	private:
		friend class EventReceiver<T>;

		void Subscribe(EventReceiver<T>& receiver, const Func& func);
		void Unsubscribe(const EventReceiver<T>& receiver);

		std::vector<Func> m_funcs;
		std::vector<EventReceiver<T>*> m_receivers;
	};

	template<>
	class EventDispatcher<void>
	{
	public:
		using Func = std::function<void()>;

		~EventDispatcher()
		{
			Clear();
		}

		void Broadcast();
		void Clear();

	private:
		friend class EventReceiver<void>;
		void Subscribe(EventReceiver<void>& receiver, const Func& func);
		void Unsubscribe(const EventReceiver<void>& receiver);

		std::vector<Func> m_funcs;
		std::vector<EventReceiver<void>*> m_receivers;
	};


	// EventReceiver

	template<typename T>
	void EventReceiver<T>::Subscribe(EventDispatcher<T>& dispatcher, const Func& func)
	{
		if (!m_subscribed)
		{
			m_dispatcher = &dispatcher;
			m_dispatcher->Subscribe(*this, func);
			m_subscribed = true;
		}
		else
		{
			BF_CORE_LOG_WARN("Already subscribed to an event dispatcher.");
		}
	}

	template<typename T>
	void EventReceiver<T>::Unsubscribe()
	{
		if (m_subscribed && m_dispatcher)
		{
			m_dispatcher->Unsubscribe(*this);
			m_subscribed = false;
			m_dispatcher = nullptr;
		}
		else
		{
			BF_CORE_LOG_WARN("Not subscribed to any event dispatcher.");
		}
	}

	template<typename T>
	void EventReceiver<T>::InvalidateDispatcher()
	{
		m_dispatcher = nullptr;
		m_subscribed = false;
	}


	// EventReceiver<void>

	inline void EventReceiver<void>::Subscribe(EventDispatcher<void>& dispatcher, const Func& func)
	{
		if (!m_subscribed)
		{
			m_dispatcher = &dispatcher;
			m_dispatcher->Subscribe(*this, func);
			m_subscribed = true;
		}
		else
		{
			BF_CORE_LOG_WARN("Already subscribed to an event dispatcher.");
		}
	}

	inline void EventReceiver<void>::Unsubscribe()
	{
		if (m_subscribed && m_dispatcher)
		{
			m_dispatcher->Unsubscribe(*this);
			m_subscribed = false;
			m_dispatcher = nullptr;
		}
		else
		{
			BF_CORE_LOG_WARN("Not subscribed to any event dispatcher.");
		}
	}

	inline void EventReceiver<void>::InvalidateDispatcher()
	{
		m_dispatcher = nullptr;
		m_subscribed = false;
	}


	// EventDispatcher

	template<typename T>
	void EventDispatcher<T>::Subscribe(EventReceiver<T>& receiver, const Func& func)
	{
		m_funcs.push_back(func);
		m_receivers.push_back(&receiver);

		BF_CORE_LOG_TRACE("EventDispatcher::Subscribe: %s", typeid(receiver).name());
	}

	template<typename T>
	void EventDispatcher<T>::Unsubscribe(const EventReceiver<T>& receiver)
	{
		for (int i = 0; i < m_funcs.size(); ++i)
		{
			if (&receiver == m_receivers[i])
			{
				m_receivers[i]->InvalidateDispatcher();

				m_funcs.erase(m_funcs.begin() + i);
				m_receivers.erase(m_receivers.begin() + i);

				BF_CORE_LOG_TRACE("EventDispatcher::Unsubscribe: %s", typeid(receiver).name());
				break;
			}
		}
	}

	template<typename T>
	void EventDispatcher<T>::Broadcast(const T& val)
	{
		for (auto& func : m_funcs)
		{
			func(val);
		}
	}

	template<typename T>
	void EventDispatcher<T>::Clear()
	{
		for (auto* receiver : m_receivers)
		{
			receiver->InvalidateDispatcher();
		}

		m_funcs.clear();
		m_receivers.clear();
	}

	// EventDispatcher<void>

	inline void EventDispatcher<void>::Subscribe(EventReceiver<void>& receiver, const Func& func)
	{
		m_funcs.push_back(func);
		m_receivers.push_back(&receiver);

		BF_CORE_LOG_TRACE("EventDispatcher::Subscribe: %s", typeid(receiver).name());
	}

	inline void EventDispatcher<void>::Unsubscribe(const EventReceiver<void>& receiver)
	{
		for (int i = 0; i < m_funcs.size(); ++i)
		{
			if (&receiver == m_receivers[i])
			{
				m_receivers[i]->InvalidateDispatcher();

				m_funcs.erase(m_funcs.begin() + i);
				m_receivers.erase(m_receivers.begin() + i);

				BF_CORE_LOG_TRACE("EventDispatcher::Unsubscribe: %s", typeid(receiver).name());
				break;
			}
		}
	}

	inline void EventDispatcher<void>::Broadcast()
	{
		for (auto& func : m_funcs)
		{
			func();
		}
	}

	inline void EventDispatcher<void>::Clear()
	{
		for (auto* receiver : m_receivers)
		{
			receiver->InvalidateDispatcher();
		}

		m_funcs.clear();
		m_receivers.clear();
	}
}