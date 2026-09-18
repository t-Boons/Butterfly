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

	void SkyboxComponent::SetTextureUUIDPositiveRight(const UUID& uuid)
	{
		m_serializeUUIDs[0] = uuid;
		Application::Get().GetAssetManager().Acquire(uuid, m_textureHandles[0]);
		MarkDirty();
	}

	void SkyboxComponent::SetTextureUUIDPositiveLeft(const UUID& uuid)
	{
		m_serializeUUIDs[1] = uuid;
		Application::Get().GetAssetManager().Acquire(uuid, m_textureHandles[1]);
		MarkDirty();
	}

	void SkyboxComponent::SetTextureUUIDPositiveTop(const UUID& uuid)
	{
		m_serializeUUIDs[2] = uuid;
		Application::Get().GetAssetManager().Acquire(uuid, m_textureHandles[2]);
		MarkDirty();
	}

	void SkyboxComponent::SetTextureUUIDPositiveBottom(const UUID& uuid)
	{
		m_serializeUUIDs[3] = uuid;
		Application::Get().GetAssetManager().Acquire(uuid, m_textureHandles[3]);
		MarkDirty();
	}

	void SkyboxComponent::SetTextureUUIDPositiveFront(const UUID& uuid)
	{
		m_serializeUUIDs[4] = uuid;
		Application::Get().GetAssetManager().Acquire(uuid, m_textureHandles[4]);
		MarkDirty();
	}

	void SkyboxComponent::SetTextureUUIDPositiveBack(const UUID& uuid)
	{
		m_serializeUUIDs[5] = uuid;
		Application::Get().GetAssetManager().Acquire(uuid, m_textureHandles[5]);
		MarkDirty();
	}
}