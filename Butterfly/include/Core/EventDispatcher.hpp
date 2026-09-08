#pragma once
#include <functional>
#include <vector>

namespace Butterfly
{
	template <typename InType = void>
	class EventDispatcher
	{
	public:
		using Func = std::function<void(const InType&)>;

		void Broadcast(const InType& val)
		{
			BF_PROFILE_EVENT();

			for (auto& f : m_funcs)
				f(val);
		}

		void Subscribe(const Func& func)
		{
			BF_PROFILE_EVENT();

			m_funcs.push_back(func);
		}

	private:
		std::vector<Func> m_funcs;
	};


	template <>
	class EventDispatcher<void>
	{
	public:
		using Func = std::function<void()>;

		void Broadcast()
		{
			BF_PROFILE_EVENT();

			for (auto& f : m_funcs)
				f();
		}

		void Subscribe(const Func& func)
		{
			BF_PROFILE_EVENT();

			m_funcs.push_back(func);
		}

	private:
		std::vector<Func> m_funcs;
	};
}