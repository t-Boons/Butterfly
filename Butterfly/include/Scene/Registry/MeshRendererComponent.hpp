#pragma once
#include "Core/Common.hpp"
#include "Asset/AssetTypes.hpp"

namespace Butterfly
{
	class MeshRendererComponent : public NonCopyable
	{
	public:
		bool ContainsMesh() const
		{
			return MeshHandle.Valid();
		}

		AssetHandle<MeshAsset> MeshHandle;
	};
}
