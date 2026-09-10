#include "Core/UUID.hpp"
#include <random>

namespace Butterfly
{
	UUID UUID::Generate()
	{
        static std::random_device rd;
        static std::mt19937_64 generator(((static_cast<uint64_t>(rd()) << 32) | rd()));

        UUID uuid;
        uuid.m_high = generator();
        uuid.m_low = generator();

        return uuid;
	}

	UUID::UUID()
		: m_high(0), m_low(0)
	{
	}

    std::string UUID::ToString() const
    {
        return std::to_string(m_low) + "-" + std::to_string(m_high);
    }

    UUID UUID::FromString(const std::string& str)
    {
        UUID uuid;
        const size_t separator = str.find('-');

        BF_CORE_ASSERT(separator != std::string::npos, "Invalid UUID");

        uuid.m_low = std::stoull(str.substr(0, separator));
        uuid.m_high = std::stoull(str.substr(separator + 1));
        return uuid;
    }

    bool UUID::Valid() const
    {
        return m_high != 0 || m_low != 0;
    }

    bool UUID::operator==(const UUID& other) const
    {
        return m_high == other.m_high && m_low == other.m_low;
    }

    UUID::operator bool() const
    {
        return Valid();
    }
}