#include "Asset/AssetManager.hpp"

namespace Butterfly
{
	AssetManager::AssetManager()
        : m_assetRegistry(this)
    {
    }

	void AssetManager::Tick()
	{
		if (m_tickCounter % 60 == 0)
		{
			GarbageCollect();
		}

		m_tickCounter++;
	}

	void AssetManager::GarbageCollect()
	{
		for (auto it = m_entries.begin(); it != m_entries.end();)
		{
			if (it->second.RefCount == 0)
			{
				BF_CORE_LOG_TRACE("Garbage collecting asset: %s", it->first.ToString().c_str());
				it = m_entries.erase(it);
			}
			else
			{
				it++;
			}
		}
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