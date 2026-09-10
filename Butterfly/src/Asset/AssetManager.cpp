#include "Asset/AssetManager.hpp"
#include "Asset/Importer/OBJImporter.hpp"

namespace Butterfly
{
	AssetManager::AssetManager()
        : m_assetRegistry(this)
    {
        m_importers.push_back(MakeRef<OBJImporter>());
    }

	void AssetManager::AddRef(const UUID& id)
	{
		auto it = m_entries.find(id);
		BF_CORE_ASSERT(it != m_entries.end(), "Trying to add ref to non-existing asset");
		it->second.RefCount++;
		BF_CORE_LOG_TRACE("RefCount ++ asset: %s is %d", id.ToString().c_str(), it->second.RefCount);
	}

	void AssetManager::SubtractRef(const UUID& id)
	{
		auto it = m_entries.find(id);
		BF_CORE_ASSERT(it != m_entries.end(), "Trying to subtract ref from non-existing asset");
		BF_CORE_ASSERT(it->second.RefCount > 0, "Trying to subtract ref from asset with 0 refs");

		it->second.RefCount--;
		BF_CORE_LOG_TRACE("RefCount -- asset: %s is %d", id.ToString().c_str(), it->second.RefCount);
	}
}