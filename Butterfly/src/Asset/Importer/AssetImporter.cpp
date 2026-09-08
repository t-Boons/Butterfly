#include "Asset/Importer/AssetImporter.hpp"
#include "Asset/Importer/OBJImporter.hpp"
#include "Core/Application.hpp"
#include "Asset/AssetRegistry.hpp"

namespace Butterfly
{
	std::vector<RefPtr<IAssetImporter>> IAssetImporter::CreateImporters()
	{
		std::vector<RefPtr<IAssetImporter>> importers;

		importers.push_back(MakeRef< OBJImporter>());

		return importers;
	}
}