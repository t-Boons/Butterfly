#pragma once
#include "Core/Common.hpp"
#include "Asset/AssetHandle.hpp"
#include "Renderer/D3D12Buffer.hpp"
#include "Renderer/D3D12Texture.hpp"
#include "Renderer/RenderIncludes.hpp"

namespace Butterfly
{
	struct MaterialAsset;

	struct MeshAsset
	{
		inline static const AssetType Type() { return { "Mesh" }; }

		std::string Name;

		std::vector<glm::vec3> Positions;
		std::vector<glm::vec3> Normals;
		std::vector<glm::vec2> UVs;
		std::vector<glm::vec4> Tangents;
		std::vector<uint32_t> Indices;

		RefPtr<BFStructuredBuffer> GPUPositions;
		RefPtr<BFStructuredBuffer> GPUNormals;
		RefPtr<BFStructuredBuffer> GPUTangents;
		RefPtr<BFStructuredBuffer> GPUUVs;
		RefPtr<BFIndexBuffer> GPUIndices;

		Bounds Bounds;
		glm::vec3 SDFResolution = { 0.0f , 0.0f, 0.0f};
		RefPtr<BFTexture> SDF;


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
				desc.Data = Tangents.data();
				desc.NumElements = static_cast<uint32_t>(Tangents.size());
				desc.Stride = sizeof(glm::vec4);
				desc.DebugName = "Tangents";
				GPUTangents = MakeRef<BFStructuredBuffer>(desc);
			}
			{
				desc.Data = UVs.data();
				desc.NumElements = static_cast<uint32_t>(UVs.size());
				desc.Stride = sizeof(glm::vec2);
				desc.DebugName = "UVs";
				GPUUVs = MakeRef<BFStructuredBuffer>(desc);
			}

			GPULoaded = true;

			for (auto& subMesh : SubMeshes)
			{
				for (uint32_t i = 0; i < subMesh.IndexCount; ++i)
				{
					uint32_t index = Indices[subMesh.IndexOffset + i];

					const glm::vec3 position = Positions[index];
					if (Bounds.Min.x > position.x) Bounds.Min.x = position.x;
					if (Bounds.Min.y > position.y) Bounds.Min.y = position.y;
					if (Bounds.Min.z > position.z) Bounds.Min.z = position.z;
					if (Bounds.Max.x < position.x) Bounds.Max.x = position.x;
					if (Bounds.Max.y < position.y) Bounds.Max.y = position.y;
					if (Bounds.Max.z < position.z) Bounds.Max.z = position.z;
				}
			}
			const float padding = 0.05f;
			Bounds.Min -= glm::vec3(padding);
			Bounds.Max += glm::vec3(padding);
			SDFResolution = glm::vec3(128.0f, 128.0f, 128.0f);
			D3D12CommandList list;

			std::vector<SDFTriangle> triangles;
			triangles.reserve(Indices.size() / 3);

			for (auto& subMesh : SubMeshes)
			{
				for (uint32_t i = 0; i < subMesh.IndexCount; i += 3)
				{
					uint32_t index0 = Indices[subMesh.IndexOffset + i];
					uint32_t index1 = Indices[subMesh.IndexOffset + i + 1];
					uint32_t index2 = Indices[subMesh.IndexOffset + i + 2];
					SDFTriangle triangle;
					triangle.Vertex0 = Positions[index0];
					triangle.Vertex1 = Positions[index1];
					triangle.Vertex2 = Positions[index2];
					triangles.push_back(triangle);
				}
			}

			SDF = GraphicsCommands::CreateSDF(triangles, Bounds, SDFResolution);
		}
	};


	struct TextureAsset
	{
		inline static const AssetType Type() { return { "Texture" }; }
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
		inline static const AssetType Type() { return { "Model" }; }
		RefPtr<ModelNode> RootNode;
	};

	struct MaterialAsset
	{
		inline static const AssetType Type() { return { "Material" }; }

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