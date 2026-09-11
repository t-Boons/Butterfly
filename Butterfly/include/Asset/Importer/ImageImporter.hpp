#pragma once
#include "Asset/Importer/AssetImporter.hpp"

namespace Butterfly
{
	class ImageImporter : public IAssetImporter
	{
	public:
		virtual bool Import(const AssetMetadata& path, ImportResult& ret) const override;
		virtual bool CanImport(const std::string& fileExtention) const override;
		virtual bool CanImportType(const std::type_info& type) const override;
	};
}