#pragma once
#include "Core/Common.hpp"
#include "Asset/Asset.hpp"

namespace Butterfly
{
#define BF_REGISTER_ASSET_IMPORTER(type)				\
	template<typename T>								\
	struct ImporterRegistrar							\
	{													\
		ImporterRegistrar()								\
		{												\
			AssetManager::RegisterImporter<T>();		\
		}												\
	};													\
														\
	static ImporterRegistrar<type> s_##type##Importer;

	class AssetManager;
	class IAssetImporter
	{
	public:
		virtual bool CreateMeta(const std::filesystem::path& file, AssetFileMetadata& meta) const = 0;
		virtual bool Import(const AssetFileMetadata& meta, AssetManager& manager) const = 0;
		virtual bool CanImport(const std::string& fileExtention) const = 0;
		virtual bool CanImportType(const AssetType& type) const = 0;
	};
}