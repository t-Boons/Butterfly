#pragma once
#include "Core/Common.hpp"
#include "Asset/AssetHandle.hpp"
#include "Renderer/D3D12Buffer.hpp"
#include "Renderer/D3D12Texture.hpp"

namespace Butterfly
{
	struct MaterialAsset;

	struct MeshAsset
	{
		inline static const AssetType Type = { "Mesh" };

		std::string Name;

		std::vector<glm::vec3> Positions;
		std::vector<glm::vec3> Normals;
		std::vector<glm::vec2> UVs;
		std::vector<glm::vec4> Tangents;
		std::vector<glm::vec3> Bitangents;
		std::vector<uint32_t> Indices;

		RefPtr<BFStructuredBuffer> GPUPositions;
		RefPtr<BFStructuredBuffer> GPUNormals;
		RefPtr<BFStructuredBuffer> GPUUVs;
		RefPtr<BFIndexBuffer> GPUIndices;


		struct SubMesh
		{
			uint32_t IndexOffset;
			uint32_t IndexCount;
			AssetHandle<MaterialAsset> Material;
		};

		std::vector<SubMesh> SubMeshes;

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
				desc.DebugName = "UVs";
				GPUUVs = MakeRef<BFStructuredBuffer>(desc);
			}

			GPULoaded = true;
		}
	};


	struct TextureAsset
	{
		inline static const AssetType Type = { "Texture" };
		RefPtr<BFTexture> Texture;
	};

	struct ModelNode
	{
		std::string Name;
		glm::mat4 ModelMatrix = glm::mat4(1.0f);
		std::vector<RefPtr<ModelNode>> Children;

		AssetHandle<MeshAsset> Mesh;
	};

	struct ModelAsset
	{
		inline static const AssetType Type = { "Model" };
		RefPtr<ModelNode> RootNode;
	};

	struct MaterialAsset
	{
		inline static const AssetType Type = { "Material" };

		float Metallic = 0.5f;
		float Roughness = 0.5f;
		glm::vec4 BaseColor = glm::vec4(1.0f);
		glm::vec4 EmissiveColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		AssetHandle<TextureAsset> ColorTexture;
		AssetHandle<TextureAsset> NormalTexture;
		AssetHandle<TextureAsset> MetallicRoughnessTexture;
		AssetHandle<TextureAsset> EmissionTexture;
		AssetHandle<TextureAsset> AmbientOcclusionTexture;
		float NormalScale = 1.0f;

		std::string Name;
	};
}