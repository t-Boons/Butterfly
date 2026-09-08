#pragma once
#include "Core/Common.hpp"
#include "Asset/Asset.hpp"

namespace Butterfly
{
	class IAssetImporter
	{
	public:
		static std::vector<RefPtr<IAssetImporter>> CreateImporters();

		virtual RefPtr<IAsset> Import(const std::filesystem::path& path) = 0;
		virtual bool IsCorrectFileExtention(const std::string& fileExtention) = 0;
		virtual std::string_view AssetType() = 0;
	};
}