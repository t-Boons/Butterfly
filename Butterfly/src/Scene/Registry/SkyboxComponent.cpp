#include "Scene/Registry/SkyboxComponent.hpp"
#include "Core/Application.hpp"

namespace Butterfly
{
	SkyboxComponent::SkyboxComponent(const Entity& entity)
	{
	}

	void SkyboxComponent::SetTextureHandle(uint32_t side, const AssetHandle<TextureAsset>& handle)
	{
		BF_CORE_ASSERT(side < 6, "SkyboxComponent::SetTextureHandle: Invalid side index for skybox: %u", side);
		m_textureHandles[side] = handle;
		m_serializeUUIDs[side] = handle.GetID();
		MarkDirty();
	}

	const AssetHandle<TextureAsset>& SkyboxComponent::GetTextureHandle(uint32_t side) const
	{
		BF_CORE_ASSERT(side < 6, "SkyboxComponent::GetTextureHandle: Invalid side index for skybox: %u", side);
		return m_textureHandles[side];
	}

	void SkyboxComponent::SetTextureUUIDRight(const AssetUUID<TextureAsset>& uuid)
	{
		if(Application::Get().GetAssetManager().Acquire(uuid, m_textureHandles[0]))
		{
			m_serializeUUIDs[0] = uuid;
			MarkDirty();
		}
	}

	void SkyboxComponent::SetTextureUUIDLeft(const AssetUUID<TextureAsset>& uuid)
	{
		if(Application::Get().GetAssetManager().Acquire(uuid, m_textureHandles[1]))
		{
			m_serializeUUIDs[1] = uuid;
			MarkDirty();
		}
	}

	void SkyboxComponent::SetTextureUUIDTop(const AssetUUID<TextureAsset>& uuid)
	{
		if(Application::Get().GetAssetManager().Acquire(uuid, m_textureHandles[2]))
		{
			m_serializeUUIDs[2] = uuid;
			MarkDirty();
		}
	}

	void SkyboxComponent::SetTextureUUIDBottom(const AssetUUID<TextureAsset>& uuid)
	{
		if(Application::Get().GetAssetManager().Acquire(uuid, m_textureHandles[3]))
		{
			m_serializeUUIDs[3] = uuid;
			MarkDirty();
		}
	}

	void SkyboxComponent::SetTextureUUIDFront(const AssetUUID<TextureAsset>& uuid)
	{
		if(Application::Get().GetAssetManager().Acquire(uuid, m_textureHandles[4]))
		{
			m_serializeUUIDs[4] = uuid;
			MarkDirty();
		}
	}

	void SkyboxComponent::SetTextureUUIDBack(const AssetUUID<TextureAsset>& uuid)
	{
		if(Application::Get().GetAssetManager().Acquire(uuid, m_textureHandles[5]))
		{
			m_serializeUUIDs[5] = uuid;
			MarkDirty();
		}
	}
}