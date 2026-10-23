#pragma once
#include "Core/Common.hpp"
#include "Asset/AssetHandle.hpp"
#include "Renderer/D3D12Buffer.hpp"
#include "Renderer/D3D12Texture.hpp"
#include "Renderer/RenderIncludes.hpp"

namespace Butterfly
{
	struct MaterialAsset;
	struct BFTexture;
	struct SDF;

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


		Bounds m_meshBounds;

		struct SubMesh
		{
			uint32_t IndexOffset;
			uint32_t IndexCount;
			AssetHandle<MaterialAsset> Material;
		};
		std::vector<SubMesh> SubMeshes;


		Bounds GetBounds() const { return m_meshBounds; }
		const RefPtr<SDF>& GetSDF() const { return m_sdf; }

		void LoadToGPU();

		bool m_gpuLoaded = false;
		bool CreateSDF = false;
		RefPtr<SDF> m_sdf;
	};

	struct SDF
	{
	public:
		void GenerateSDF(const std::vector<glm::vec3>& positions, const std::vector<uint32_t>& indices, const std::vector<MeshAsset::SubMesh>& subMeshes, const Bounds& meshBounds);

		const RefPtr<BFTexture>& GetSDFTexture() const { return m_sdf; }
		const glm::vec3& GetSDFResolution() const { return m_sdfResolution; }
		const Bounds& GetSDFBounds() const { return m_sdfBounds; }

	private:
		RefPtr<BFTexture> m_sdf;
		glm::vec3 m_sdfResolution;
		Bounds m_sdfBounds;
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