#pragma once
#include "Asset/Importer/AssetImporter.hpp"

namespace Butterfly
{
	class AssetManager;
	class GLTFImporter : public IAssetImporter
	{
	public:
		virtual bool Import(const AssetMetadata& path, AssetManager& manager) const override;
		virtual bool CanImport(const std::string& fileExtention) const override;
		virtual bool CanImportType(const std::type_info& type) const override;
	};
}