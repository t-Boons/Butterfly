#pragma once
#include "Core/Common.hpp"
#include "Core/UUID.hpp"

namespace Butterfly
{
	struct AssetType
	{
		bool operator ==(const AssetType& other) const
		{
			return TypeName == other.TypeName;
		}

		std::string TypeName;
	};

	struct AssetMetadata
	{
		UUID SourceFileID;
		std::string Name;
		AssetType Type;
		UUID AssetID;
	};

	struct AssetFileMetadata
	{
		void SyncSourceFileIDWithAssets()
		{
			for (auto& [id, asset] : Assets)
			{
				asset.SourceFileID = SourceFileID;
			}
		}

		std::filesystem::path Path;
		UUID SourceFileID;
		UUID RootAssetID;

		std::unordered_map<UUID, AssetMetadata> Assets;
	};

	struct AssetEntry
	{
		AssetEntry() = default;
		AssetEntry(const UUID& id, const AssetType& type, RefPtr<void> data, bool persistant = false)
			: ID(id), Type(type), Data(data), Persistant(persistant)
		{
		}

		UUID ID;
		AssetType Type;
		RefPtr<void> Data;
		uint32_t RefCount = 0;
		bool Persistant = false;
	};
}

