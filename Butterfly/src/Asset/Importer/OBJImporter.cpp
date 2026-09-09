#include "Asset/Importer/OBJImporter.hpp"

namespace Butterfly
{
	bool OBJImporter::CanImport(const std::string& fileExtention) const
	{
		return fileExtention == ".obj";
	}

	ImportResult OBJImporter::Import(const AssetMetadata& path)
	{
		ImportResult ret;
		return ret;
	}
}