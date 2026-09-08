#pragma once
#include "Core/Common.hpp"
#include "Asset/Asset.hpp"

namespace Butterfly
{
	class IAssetImporter
	{
	public:
		static RefPtr<IAssetImporter> FindImporter(const std::filesystem::path& path);
		virtual RefPtr<IAsset> Import(const std::filesystem::path& path) = 0;
	};
}