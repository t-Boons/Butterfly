#pragma once
#include "Core/Common.hpp"

namespace Butterfly
{
	struct UUID
	{
		static UUID Generate();

	private:
		uint64_t m_high;
		uint64_t m_low;
	};
}