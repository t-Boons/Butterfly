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


	struct ImportedAsset
	{
		AssetType Type;
		std::shared_ptr<void> Data;
	};

	struct ImportResult
	{
		ImportedAsset Asset;
	};

	class IAssetImporter
	{
	public:
		virtual bool Import(const AssetMetadata& path, ImportResult& ret) const = 0;
		virtual bool CanImport(const std::string& fileExtention) const = 0;
		virtual bool CanImportType(const std::type_info& type) const = 0;
	};
}