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
		bool ImportFromDisk(const std::filesystem::path& file, AssetFileMetadata& meta);
		bool NewFile(const std::string& name, const std::string& extention, const std::string& contents, AssetFileMetadata& meta);

		void Scan();
		bool FindAsset(const UUID& id, AssetMetadata& meta) const;
		bool FindFile(const UUID& id, AssetFileMetadata& fileMeta) const;
		const std::unordered_map<UUID, AssetFileMetadata>& GetAll() const { return m_registeredFiles; }

	private:
		AssetFileMetadata WriteNewMetaForFile(const std::filesystem::path& file) const;
		AssetFileMetadata ReadMetaFromFile(const std::filesystem::path& file) const;

		void Register(const AssetFileMetadata& meta);

		AssetManager* m_manager;
		std::unordered_map<UUID, AssetFileMetadata> m_registeredFiles;
		std::unordered_map<UUID, AssetMetadata> m_registeredAssets;
		std::filesystem::path m_assetPath;
		const std::string m_metaFileExtention = ".meta";
	};
}