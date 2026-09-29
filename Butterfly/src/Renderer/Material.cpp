#include "Renderer/Material.hpp"

namespace Butterfly
{
	namespace Utils
	{
		MaterialData MaterialAssetToMaterialData(const UUID& id, AssetManager& manager)
		{
			MaterialData data;
			MaterialAsset* material = manager.ResolveWeak<MaterialAsset>(id);
			if (!material)
			{
				BF_CORE_LOG_ERROR("Failed to resolve MaterialAsset with ID: %s", id.ToString().c_str());
				return data;
			}
			data.BaseColor = material->BaseColor;
			data.EmissiveColor = material->EmissiveColor;
			data.Metallic = material->Metallic;
			data.Roughness = material->Roughness;
			data.NormalScale = material->NormalScale;
			if (material->ColorTexture.Valid())
			{
				TextureAsset* tex = manager.Resolve<TextureAsset>(material->ColorTexture);
				data.ColorTexture = tex->Texture->SRV().View();
			}
			else
			{
				data.ColorTexture = -1;
			}
			if (material->NormalTexture.Valid())
			{
				TextureAsset* tex = manager.Resolve<TextureAsset>(material->NormalTexture);
				data.NormalTexture = tex->Texture->SRV().View();
			}
			else
			{
				data.NormalTexture = -1;
			}
			if (material->MetallicRoughnessTexture.Valid())
			{
				TextureAsset* tex = manager.Resolve<TextureAsset>(material->MetallicRoughnessTexture);
				data.MetallicRoughnessTexture = tex->Texture->SRV().View();
			}
			else
			{
				data.MetallicRoughnessTexture = -1;
			}
			if (material->EmissionTexture.Valid())
			{
				TextureAsset* tex = manager.Resolve<TextureAsset>(material->EmissionTexture);
				data.EmissionTexture = tex->Texture->SRV().View();
			}
			else
			{
				data.EmissionTexture = -1;
			}
			if (material->AmbientOcclusionTexture.Valid())
			{
				TextureAsset* tex = manager.Resolve<TextureAsset>(material->AmbientOcclusionTexture);
				data.AmbientOcclusionTexture = tex->Texture->SRV().View();
			}
			else
			{
				data.AmbientOcclusionTexture = -1;
			}
			return data;
		}
	}

	MaterialLibrary::MaterialLibrary()
	{
		// Add Fallback material.
		UUID id; // Invalid UUID.
		m_materialIndexMap.insert({ id, static_cast<uint32_t>(m_materialData.size()) });

		MaterialData data = MaterialData();
		m_materialData.push_back(data);
		m_materialUUIDs.push_back(id);


		std::vector<UUID> uuids;
		Application::Get().GetAssetManager().GetAllAssetsOfType(MaterialAsset::Type(), uuids);
		for (auto& uuid : uuids)
		{
			m_newMaterials.push(uuid);
		}

		BFStructuredBufferDesc desc;
		desc.NumElements = 128;
		desc.Stride = sizeof(MaterialData);
		desc.HeapType = BFHeapType::Upload;
		desc.DebugName = "Materials";
		m_materialBuffer = MakeRef<BFStructuredBuffer>(desc);
		m_materialBuffer->Write(m_materialData.data(), static_cast<uint32_t>(m_materialData.size() * sizeof(MaterialData)));

		m_assetAddedReceiver.Subscribe(Application::Get().GetAssetManager().OnAssetAddedEvent(), BF_BIND_FUNC_PARAM(&MaterialLibrary::RegisterMaterial));
		m_assetRemovedReceiver.Subscribe(Application::Get().GetAssetManager().OnAssetRemovedEvent(), BF_BIND_FUNC_PARAM(&MaterialLibrary::UnRegisterMaterial));
	}

	void MaterialLibrary::RegisterMaterial(const AssetEvent& ev)
	{
		if (ev.Type == MaterialAsset::Type())
		{
			m_newMaterials.push(ev.ID);
		}
	}

	void MaterialLibrary::UnRegisterMaterial(const AssetEvent& ev)
	{
		if (ev.Type == MaterialAsset::Type())
		{
			m_removedMaterials.push(ev.ID);
		}
	}

	void MaterialLibrary::Validate()
	{
		while (!m_newMaterials.empty())
		{
			const UUID id = m_newMaterials.front();
			m_newMaterials.pop();

			if (m_materialIndexMap.find(id) != m_materialIndexMap.end())
			{
				continue;
			}

			m_materialIndexMap.insert({ id, static_cast<uint32_t>(m_materialData.size()) });

			MaterialData data = Utils::MaterialAssetToMaterialData(id, Application::Get().GetAssetManager());
			m_materialData.push_back(data);
			m_materialUUIDs.push_back(id);

			if (!m_materialBuffer || m_materialData.size() > m_materialBuffer->NumElements())
			{
				BFStructuredBufferDesc desc;
				desc.NumElements = static_cast<uint32_t>(m_materialData.size() * 2);
				desc.Stride = sizeof(MaterialData);
				desc.HeapType = BFHeapType::Upload;
				desc.DebugName = "Materials";
				m_materialBuffer = MakeRef<BFStructuredBuffer>(desc);

			}

			m_materialBuffer->Write(m_materialData.data(), static_cast<uint32_t>(m_materialData.size() * sizeof(MaterialData)));
		}

		while (!m_removedMaterials.empty())
		{
			const UUID id = m_removedMaterials.front();
			m_removedMaterials.pop();
			auto it = m_materialIndexMap.find(id);
			if (it == m_materialIndexMap.end())
			{
				continue;
			}

			const uint32_t index = it->second;
			const uint32_t lastIndex = static_cast<uint32_t>(m_materialData.size() - 1);

			if (index != lastIndex)
			{
				m_materialData[index] = std::move(m_materialData[lastIndex]);

				const UUID movedUUID = m_materialUUIDs[lastIndex];
				m_materialUUIDs[index] = movedUUID;
				m_materialIndexMap[movedUUID] = index;
			}

			m_materialData.pop_back();
			m_materialUUIDs.pop_back();
			m_materialIndexMap.erase(it);
		}

	}

	const BFStructuredBuffer& MaterialLibrary::GetMaterialBuffer() const
	{
		return *m_materialBuffer;
	}

	int MaterialLibrary::GetFallbackMaterialIndex() const
	{
		return 0;
	}

	int MaterialLibrary::GetMaterialIndex(const UUID& id) const
	{
		auto it = m_materialIndexMap.find(id);
		if (it != m_materialIndexMap.end())
		{
			return it->second;
		}
		BF_CORE_LOG_CRITICAL("MaterialLibrary::GetMaterialIndex: Material with ID %s not found.", id.ToString().c_str());
		return -1;
	}
}