#pragma once
#include "tiny_obj_loader.h" 
#include "Asset/Importer/AssetImporter.hpp"

namespace Butterfly
{
	class OBJImporter : public IAssetImporter
	{
		virtual RefPtr<IAsset> Import(const std::filesystem::path& path);
	};
}