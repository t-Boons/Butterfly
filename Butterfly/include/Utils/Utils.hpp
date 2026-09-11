#pragma once
#include <numeric>
#include <filesystem>
#include <random>
#include <algorithm>

namespace Butterfly
{
	namespace Utils
	{
		inline std::wstring StringToWString(const std::string& str)
		{
			return std::filesystem::path(str).wstring();
		}

		constexpr uint32_t Align256(uint32_t value)
		{
			return (value + 255) & ~(255);
		}

		inline uint64_t RandomUint64()
		{
			static std::random_device rd;
			static std::mt19937_64 generator(rd());

			return generator();
		}

		template<typename Type>
		inline bool IsArrayPtrValid(uint32_t numElements, Type* ptr)
		{
			for (uint32_t i = 0; i < numElements; ++i)
			{
				if (ptr[i] == nullptr)
				{
					return false;
				}
			}

			return true;
		}

		// Function is taken from this stackoverflow question about variadic templates for hash combining:
		// https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
		template <typename...> struct Hasher;

		template<typename T>
		struct Hasher<T>
			: public std::hash<T>
		{
			using std::hash<T>::hash;
		};


		template <typename T, typename... Rest>
		struct Hasher<T, Rest...>
		{
			inline std::size_t operator()(const T& v, const Rest&... rest) {
				std::size_t seed = Hasher<Rest...>{}(rest...);
				seed ^= Hasher<T>{}(v)+0x9e3779b9 + (seed << 6) + (seed >> 2);
				return seed;
			}
		};

		constexpr Hasher<uint64_t> g_hasher;

		inline uint64_t Hash(uint64_t val)
		{
			return g_hasher(val);
		}

		inline void SumHash(uint64_t& in, uint64_t val)
		{
			in ^= g_hasher(val);
		}
	}

	class NonCopyableNonMoveable
	{
	protected:
		NonCopyableNonMoveable() = default;
		~NonCopyableNonMoveable() = default;

		NonCopyableNonMoveable(const NonCopyableNonMoveable&) = delete;
		NonCopyableNonMoveable& operator=(const NonCopyableNonMoveable&) = delete;

		NonCopyableNonMoveable(NonCopyableNonMoveable&&) = delete;
		NonCopyableNonMoveable& operator=(NonCopyableNonMoveable&&) = delete;
	};

	class NonMoveable
	{
	protected:
		NonMoveable() = default;
		~NonMoveable() = default;

		NonMoveable(const NonMoveable&) = default;
		NonMoveable& operator=(const NonMoveable&) = default;

		NonMoveable(NonMoveable&&) = delete;
		NonMoveable& operator=(NonMoveable&&) = delete;
	};

	class NonCopyable
	{
	protected:
		NonCopyable() = default;
		~NonCopyable() = default;

		NonCopyable(const NonCopyable&) = delete;
		NonCopyable& operator=(const NonCopyable&) = delete;

		NonCopyable(NonCopyable&&) noexcept = default;
		NonCopyable& operator=(NonCopyable&&) noexcept = default;
	};
}