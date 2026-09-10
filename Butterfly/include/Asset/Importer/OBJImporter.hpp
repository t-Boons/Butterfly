#pragma once
#include "Asset/Importer/AssetImporter.hpp"

namespace Butterfly
{
	class OBJImporter : public IAssetImporter
	{
	public:
		virtual bool Import(const AssetMetadata& path, ImportResult& ret) const;
		bool CanImport(const std::string& fileExtention) const override;
	};
}