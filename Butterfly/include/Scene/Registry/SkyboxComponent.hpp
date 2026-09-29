#pragma once
#include "Core/Common.hpp"
#include "Asset/AssetTypes.hpp"
#include "Asset/AssetManager.hpp"

namespace Butterfly
{
	class Entity;
	class SkyboxComponent
	{
	public:
		SkyboxComponent(const Entity& entity);
		void SetTextureHandle(uint32_t side, const AssetHandle<TextureAsset>& handle);
		const AssetHandle<TextureAsset>& GetTextureHandle(uint32_t side) const;
		bool IsDirty() const { return m_isDirty; }
		void ClearDirty() { m_isDirty = false; }

	private:
		void MarkDirty() { m_isDirty = true; }

		void SetTextureUUIDRight(const UUID& uuid);
		void SetTextureUUIDLeft(const UUID& uuid);
		void SetTextureUUIDTop(const UUID& uuid);
		void SetTextureUUIDBottom(const UUID& uuid);
		void SetTextureUUIDFront(const UUID& uuid);
		void SetTextureUUIDBack(const UUID& uuid);
		const UUID& GetTextureUUIDRight() const { return m_serializeUUIDs[0]; }
		const UUID& GetTextureUUIDLeft() const { return m_serializeUUIDs[1]; }
		const UUID& GetTextureUUIDTop() const { return m_serializeUUIDs[2]; }
		const UUID& GetTextureUUIDBottom() const { return m_serializeUUIDs[3]; }
		const UUID& GetTextureUUIDFront() const { return m_serializeUUIDs[4]; }
		const UUID& GetTextureUUIDBack() const { return m_serializeUUIDs[5]; }

		friend class ComponentRegistry;
		std::array<UUID, 6> m_serializeUUIDs;
		std::array<AssetHandle<TextureAsset>, 6> m_textureHandles;
		bool m_isDirty = true;
	};
}