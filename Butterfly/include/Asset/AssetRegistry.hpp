#pragma once
#include "Core/Common.hpp"
#include "Asset.hpp"

namespace Butterfly
{
	class AssetManager;

	class AssetRegistry : public NonCopyableNonMoveable
	{
	public:
		AssetRegistry(AssetManager* manager);
		bool ImportFromDisk(const std::filesystem::path& file, AssetMetadata& meta);

		void Scan();
		bool Find(const UUID& id, AssetMetadata& meta) const;
		const std::unordered_map<UUID, AssetMetadata>& GetAll() const { return m_registeredAssets; }
	private:
		AssetMetadata WriteNewMetaForFile(const std::filesystem::path& file) const;
		AssetMetadata ReadMetaFromFile(const std::filesystem::path& file) const;

		AssetManager* m_manager;
		std::unordered_map<UUID, AssetMetadata> m_registeredAssets;
		std::filesystem::path m_assetPath;
		const std::string m_metaFileExtention = ".meta";
	};
}