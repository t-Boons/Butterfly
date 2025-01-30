#pragma once
#include <functional>
#include <vector>

namespace Butterfly
{
	template <typename InType>
	class EventDispatcher
	{
		using Func = std::function<void(InType)>;
	public:

		void Broadcast(const InType& val)
		{
			BF_PROFILE_EVENT()

			for (auto& f : m_funcs)
			{
				f(val);
			}
		}

		void Subscribe(const Func& func)
		{
			BF_PROFILE_EVENT()

			m_funcs.push_back(func);
		}

		std::vector<Func> m_funcs;
	};
}