#include "Asset/AssetManager.hpp"
#include "Asset/Importer/OBJImporter.hpp"

namespace Butterfly
{
	AssetManager::AssetManager()
        : m_assetRegistry(this)
    {
        m_importers.push_back(MakeRef<OBJImporter>());
    }
}