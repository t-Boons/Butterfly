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
		virtual bool Import(const AssetMetadata& path, AssetManager& manager) const = 0;
		virtual bool CanImport(const std::string& fileExtention) const = 0;
		virtual bool CanImportType(const std::type_info& type) const = 0;
	};
}