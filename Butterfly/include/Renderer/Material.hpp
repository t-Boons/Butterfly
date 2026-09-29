#pragma once
#include "Core/Common.hpp"
#include "Asset/AssetManager.hpp"
#include "Asset/AssetTypes.hpp"
#include "Renderer/RenderIncludes.hpp"
#include "Core/Application.hpp"

namespace Butterfly
{
	struct MaterialData
	{
		glm::vec4 BaseColor = glm::vec4(1.0f);
		glm::vec4 EmissiveColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		float Metallic = 0.5f;
		float Roughness = 0.5f;
		float NormalScale = 1.0f;
		int ColorTexture = -1;
		int NormalTexture = -1;
		int MetallicRoughnessTexture = -1;
		int EmissionTexture = -1;
		int AmbientOcclusionTexture = -1;
	};

	class MaterialLibrary
	{
	public:
		MaterialLibrary();

		void RegisterMaterial(const AssetEvent& ev);
		void UnRegisterMaterial(const AssetEvent& ev);
		void Validate();
		const BFStructuredBuffer& GetMaterialBuffer() const;
		int GetMaterialIndex(const UUID& id) const;
		int GetFallbackMaterialIndex() const;

	private:
		std::queue<UUID> m_newMaterials;
		std::queue<UUID> m_removedMaterials;

		std::vector<MaterialData> m_materialData;
		std::vector<UUID> m_materialUUIDs;
		std::unordered_map<UUID, uint32_t> m_materialIndexMap;


		RefPtr<BFStructuredBuffer> m_materialBuffer;
		EventReceiver<AssetEvent> m_assetAddedReceiver;
		EventReceiver<AssetEvent> m_assetRemovedReceiver;
	};
}