#pragma once
#include "Core/Common.hpp"

namespace Butterfly
{
	struct UUID
	{
		static UUID Generate();
		static UUID FromString(const std::string& str);
		std::string ToString() const;

		bool operator==(const UUID& other) const {
			return m_high == other.m_high && m_low == other.m_low;
		}

	private:
		friend struct std::hash<UUID>;
		uint64_t m_high;
		uint64_t m_low;
	};
}

namespace std
{
	template<>
	struct hash<Butterfly::UUID>
	{
		size_t operator()(const Butterfly::UUID& id) const
		{
			size_t h1 = std::hash<uint64_t>()(id.m_high);
			size_t h2 = std::hash<uint64_t>()(id.m_low);
			return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
		}
	};
}