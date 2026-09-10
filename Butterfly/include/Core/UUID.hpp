#pragma once
#include "Core/Common.hpp"

namespace Butterfly
{
	struct UUID
	{
	public:
		static UUID Generate();
		static UUID FromString(const std::string& str);

		UUID();
		std::string ToString() const;
		bool Valid() const;
		bool operator==(const UUID& other) const;
		explicit operator bool() const;

	private:
		friend struct std::hash<UUID>;
		uint64_t m_high = 0;
		uint64_t m_low = 0;
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