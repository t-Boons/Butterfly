#include "Asset/Importer/OBJImporter.hpp"

namespace Butterfly
{
	bool OBJImporter::IsCorrectFileExtention(const std::string& fileExtention)
	{
		return fileExtention == ".obj";
	}

	std::string_view OBJImporter::AssetType()
	{
		return "Mesh";
	}

	RefPtr<IAsset> OBJImporter::Import(const std::filesystem::path& path)
	{
		return nullptr;
	}
}