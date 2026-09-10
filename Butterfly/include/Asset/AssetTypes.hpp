#pragma once
#include "Core/Common.hpp"
#include "Asset/Asset.hpp"
#include "Renderer/D3D12Buffer.hpp"

namespace Butterfly
{
	struct MeshAsset
	{
		std::vector<glm::vec3> Positions;
		std::vector<glm::vec3> Normals;
		std::vector<glm::vec2> UVs;
		std::vector<uint32_t> Indices;

		RefPtr<BFStructuredBuffer> GPUPositions;
		RefPtr<BFStructuredBuffer> GPUNormals;
		RefPtr<BFStructuredBuffer> GPUUVs;
		RefPtr<BFIndexBuffer> GPUIndices;

		bool GPULoaded = false;

		void GPULoad()
		{
			if (GPULoaded)
			{
				return;
			}

			GPUIndices = RefPtr<BFIndexBuffer>(new BFIndexBuffer(Indices.data(), static_cast<uint32_t>(Indices.size()), DXGI_FORMAT_R32_UINT, "Indices"));


			BFStructuredBufferDesc desc;
			desc.HeapType = BFHeapType::Default;


			{
				desc.Data = Positions.data();
				desc.NumElements = static_cast<uint32_t>(Positions.size());
				desc.Stride = sizeof(glm::vec3);
				desc.DebugName = "Positions";

				GPUPositions = MakeRef<BFStructuredBuffer>(desc);
			}

			{
				desc.Data = Normals.data();
				desc.NumElements = static_cast<uint32_t>(Normals.size());
				desc.Stride = sizeof(glm::vec3);
				desc.DebugName = "Normals";
				GPUNormals = MakeRef<BFStructuredBuffer>(desc);
			}
			{
				desc.Data = UVs.data();
				desc.NumElements = static_cast<uint32_t>(UVs.size());
				desc.Stride = sizeof(glm::vec2);
				desc.DebugName = "Positions";
				GPUUVs = MakeRef<BFStructuredBuffer>(desc);
			}

			GPULoaded = true;
		}
	};

	struct ModelAsset
	{
		std::vector<AssetHandle<MeshAsset>> Assets;
	};
}