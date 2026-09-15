#pragma once
#include "Core/Common.hpp"
#include "Asset/AssetTypes.hpp"

namespace Butterfly
{
	class SkyboxComponent
	{
	public:
		std::array<AssetHandle<TextureAsset>, 6> TextureHandle;
	};
}