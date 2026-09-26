#pragma once
#include "Asset/Importer/AssetImporter.hpp"

namespace Butterfly
{
	class AssetManager;
	class GLTFImporter : public IAssetImporter
	{
	public:
		virtual bool CreateMeta(const std::filesystem::path& file, AssetFileMetadata& meta) const override;
		virtual bool Import(const AssetFileMetadata& meta, AssetManager& manager) const override;
		virtual bool CanImport(const std::string& fileExtention) const override;
		virtual bool CanImportType(const AssetType& type) const override;
	};
}