#pragma once
#include "tinyobj/tiny_obj_loader.h" 
#include "Asset/Importer/AssetImporter.hpp"

namespace Butterfly
{
	class OBJImporter : public IAssetImporter
	{
	public:
		virtual RefPtr<IAsset> Import(const std::filesystem::path& path);
		bool IsCorrectFileExtention(const std::string& fileExtention) override;
		std::string_view AssetType() override;
	};
}