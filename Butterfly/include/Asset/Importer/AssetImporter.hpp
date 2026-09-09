#pragma once
#include "Core/Common.hpp"
#include "Asset/Asset.hpp"

namespace Butterfly
{
	struct ImportedAsset
	{
		UUID ID;
		AssetType Type;
		std::shared_ptr<void> Data;
	};

	struct ImportResult
	{
		std::vector<ImportedAsset> Assets;
	};

	class IAssetImporter
	{
	public:
		virtual ImportResult Import(const AssetMetadata& path) = 0;
		virtual bool CanImport(const std::string& fileExtention) const = 0;
	};
}