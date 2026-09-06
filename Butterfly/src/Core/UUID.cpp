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
}