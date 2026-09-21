#pragma once
#include "Core/UUID.hpp"

namespace Butterfly
{
	template<typename T>
	struct AssetUUID
	{
		AssetUUID() = default;
		AssetUUID(const UUID& uuid)
			: _UUID(uuid)
		{
		}


		void operator =(const UUID& uuid)
		{
			_UUID = uuid;
		}

		void operator =(const AssetUUID<T>& other)
		{
			_UUID = other._UUID;
		}

		const UUID& ID() const
		{
			return _UUID;
		}

		UUID& IDRef()
		{
			return _UUID;
		}

		std::string ToString() const
		{
			return _UUID.ToString();
		}


		bool operator ==(const AssetUUID<T>& other) const
		{
			return _UUID == other._UUID;
		}

	private:
		UUID _UUID;
	};
}
namespace std
{
	template<typename T>
	struct hash<Butterfly::AssetUUID<T>>
	{
		size_t operator()(const Butterfly::AssetUUID<T>& id) const
		{
			return std::hash<Butterfly::UUID>()(id.ID());
		}
	};
}