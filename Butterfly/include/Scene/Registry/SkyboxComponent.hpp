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

		void SetTextureUUIDRight(const AssetUUID<TextureAsset>& uuid);
		void SetTextureUUIDLeft(const AssetUUID<TextureAsset>& uuid);
		void SetTextureUUIDTop(const AssetUUID<TextureAsset>& uuid);
		void SetTextureUUIDBottom(const AssetUUID<TextureAsset>& uuid);
		void SetTextureUUIDFront(const AssetUUID<TextureAsset>& uuid);
		void SetTextureUUIDBack(const AssetUUID<TextureAsset>& uuid);
		const AssetUUID<TextureAsset>& GetTextureUUIDRight() const { return m_serializeUUIDs[0]; }
		const AssetUUID<TextureAsset>& GetTextureUUIDLeft() const { return m_serializeUUIDs[1]; }
		const AssetUUID<TextureAsset>& GetTextureUUIDTop() const { return m_serializeUUIDs[2]; }
		const AssetUUID<TextureAsset>& GetTextureUUIDBottom() const { return m_serializeUUIDs[3]; }
		const AssetUUID<TextureAsset>& GetTextureUUIDFront() const { return m_serializeUUIDs[4]; }
		const AssetUUID<TextureAsset>& GetTextureUUIDBack() const { return m_serializeUUIDs[5]; }

		friend class ComponentRegistry;
		friend class Skybox;
		std::array<AssetUUID<TextureAsset>, 6> m_serializeUUIDs;
		std::array<AssetHandle<TextureAsset>, 6> m_textureHandles;
		bool m_isDirty = false;
	};
}