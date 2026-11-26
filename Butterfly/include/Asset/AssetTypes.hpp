#pragma once
#include "Core/Common.hpp"
#include "Asset/AssetHandle.hpp"
#include "Renderer/D3D12Buffer.hpp"
#include "Renderer/D3D12Texture.hpp"
#include "Renderer/RenderIncludes.hpp"
#include "Math/AABB.hpp"

namespace Butterfly
{
	struct MaterialAsset;
	struct Texture;
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

		RefPtr<StructuredBuffer> GPUPositions;
		RefPtr<StructuredBuffer> GPUNormals;
		RefPtr<StructuredBuffer> GPUTangents;
		RefPtr<StructuredBuffer> GPUUVs;
		RefPtr<IndexBuffer> GPUIndices;



		class SubMesh
		{
		public:
			uint32_t IndexOffset;
			uint32_t IndexCount;
			AssetHandle<MaterialAsset> Material;
			AABB SubBounds;
		};

		void GenerateSubMeshBounds();

		const std::vector<SubMesh>& GetSubMeshes() const { return m_subMeshes; }
		void AddSubMesh(const SubMesh& subMesh) { m_subMeshes.push_back(subMesh); }

		AABB GetBounds() const { return m_meshBounds; }
		const RefPtr<SDF>& GetSDF() const { return m_sdf; }

		void LoadToGPU();

	private:
		std::vector<SubMesh> m_subMeshes;
		AABB m_meshBounds;

		bool m_gpuLoaded = false;
		bool m_createSDF = false;
		RefPtr<SDF> m_sdf;
	};

	struct SDF
	{
	public:
		void GenerateSDF(const std::vector<glm::vec3>& positions, const std::vector<uint32_t>& indices, const std::vector<MeshAsset::SubMesh>& subMeshes, const AABB& meshBounds);

		const RefPtr<Texture>& GetSDFTexture() const { return m_sdf; }
		const glm::vec3& GetSDFResolution() const { return m_sdfResolution; }
		const AABB& GetSDFBounds() const { return m_sdfBounds; }

	private:
		RefPtr<Texture> m_sdf;
		glm::vec3 m_sdfResolution;
		AABB m_sdfBounds;
	};


	struct TextureAsset
	{
		inline static const AssetType Type() { return { "Texture" }; }
		RefPtr<Texture> Texture;
		SamplerDesc Sampler;
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