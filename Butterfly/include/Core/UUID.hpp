#pragma once
#include "Core/Common.hpp"

namespace Butterfly
{
	struct UUID
	{
		static UUID Generate();
		static UUID FromString(const std::string& str);
		std::string ToString() const;

	private:
		uint64_t m_high;
		uint64_t m_low;
	};
}