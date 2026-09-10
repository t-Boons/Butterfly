#pragma once
#include "Core/Common.hpp"
#include "Asset/Asset.hpp"

namespace Butterfly
{
	struct MeshAsset
	{
		std::vector<glm::vec3> Positions;
		std::vector<glm::vec3> Normals;
		std::vector<glm::vec2> UVs;
		std::vector<uint32_t> Indices;
	};

	struct ModelAsset
	{
		std::vector<AssetHandle<MeshAsset>> Assets;
	};
}