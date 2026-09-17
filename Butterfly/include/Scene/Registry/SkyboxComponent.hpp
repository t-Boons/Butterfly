#pragma once
#include "Core/Common.hpp"
#include "Asset/AssetTypes.hpp"
#include "Core/Application.hpp"
#include "Asset/AssetManager.hpp"

namespace Butterfly
{
	class SkyboxComponent
	{
	public:
		SkyboxComponent(const Entity& entity)
		{ }

		void SetTextureHandle(uint32_t side, const AssetHandle<TextureAsset>& handle)
		{
			BF_CORE_ASSERT(side < 6, "SkyboxComponent::SetTextureHandle: Invalid side index for skybox: %u", side);
			m_textureHandles[side] = handle;
			m_serializeUUIDs[side] = handle.GetID();
			MarkDirty();
		}

		const AssetHandle<TextureAsset>& GetTextureHandle(uint32_t side) const
		{
			BF_CORE_ASSERT(side < 6, "SkyboxComponent::GetTextureHandle: Invalid side index for skybox: %u", side);
			return m_textureHandles[side];
		}

		bool IsDirty() const { return m_isDirty; }
		void ClearDirty() { m_isDirty = false; }

	private:
		void MarkDirty()
		{
			m_isDirty = true;
		}

		void SetTextureUUIDPositiveRight(const UUID& uuid)
		{
			m_serializeUUIDs[0] = uuid;
			Application::Get().GetAssetManager().Acquire(uuid, m_textureHandles[0]);
			MarkDirty();
		}

		void SetTextureUUIDPositiveLeft(const UUID& uuid)
		{
			m_serializeUUIDs[1] = uuid;
			Application::Get().GetAssetManager().Acquire(uuid, m_textureHandles[1]);
			MarkDirty();
		}
		void SetTextureUUIDPositiveTop(const UUID& uuid)
		{
			m_serializeUUIDs[2] = uuid;
			Application::Get().GetAssetManager().Acquire(uuid, m_textureHandles[2]);
			MarkDirty();
		}
		void SetTextureUUIDPositiveBottom(const UUID& uuid)
		{
			m_serializeUUIDs[3] = uuid;
			Application::Get().GetAssetManager().Acquire(uuid, m_textureHandles[3]);
			MarkDirty();
		}
		void SetTextureUUIDPositiveFront(const UUID& uuid)
		{
			m_serializeUUIDs[4] = uuid;
			Application::Get().GetAssetManager().Acquire(uuid, m_textureHandles[4]);
			MarkDirty();
		}
		void SetTextureUUIDPositiveBack(const UUID& uuid)
		{
			m_serializeUUIDs[5] = uuid;
			Application::Get().GetAssetManager().Acquire(uuid, m_textureHandles[5]);
			MarkDirty();
		}

		const UUID& GetTextureUUIDPositiveRight() const { return m_serializeUUIDs[0]; }
		const UUID& GetTextureUUIDPositiveLeft() const { return m_serializeUUIDs[1]; }
		const UUID& GetTextureUUIDPositiveTop() const { return m_serializeUUIDs[2]; }
		const UUID& GetTextureUUIDPositiveBottom() const { return m_serializeUUIDs[3]; }
		const UUID& GetTextureUUIDPositiveFront() const { return m_serializeUUIDs[4]; }
		const UUID& GetTextureUUIDPositiveBack() const { return m_serializeUUIDs[5]; }

		friend class ComponentRegistry;
		friend class Skybox;
		std::array<UUID, 6> m_serializeUUIDs;
		std::array<AssetHandle<TextureAsset>, 6> m_textureHandles;
		bool m_isDirty = false;
	};
}