#pragma once
#include "Core/Common.hpp"
#include "Asset/Asset.hpp"

namespace Butterfly
{
	struct ImportedAsset
	{
		AssetType Type;
		std::shared_ptr<void> Data;
	};

	struct ImportResult
	{
		ImportedAsset Asset;
	};

	class IAssetImporter
	{
	public:
		virtual bool Import(const AssetMetadata& path, ImportResult& ret) const = 0;
		virtual bool CanImport(const std::string& fileExtention) const = 0;
	};
}