#pragma once
#include "tinyobj/tiny_obj_loader.h" 
#include "Asset/Importer/AssetImporter.hpp"

namespace Butterfly
{
	class OBJImporter : public IAssetImporter
	{
	public:
		virtual ImportResult Import(const AssetMetadata& path);
		bool CanImport(const std::string& fileExtention) const override;
	};
}