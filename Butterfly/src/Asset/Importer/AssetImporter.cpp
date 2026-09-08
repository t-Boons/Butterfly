#include "Asset/Importer/AssetImporter.hpp"
#include "Asset/Importer/OBJImporter.hpp"

namespace Butterfly
{
	RefPtr<IAssetImporter> IAssetImporter::FindImporter(const std::filesystem::path& path)
	{
		const std::string extention = path.extension();

		if (extention == ".obj")
		{

		}
	}
}