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

		void SetTextureUUIDPositiveRight(const UUID& uuid);
		void SetTextureUUIDPositiveLeft(const UUID& uuid);
		void SetTextureUUIDPositiveTop(const UUID& uuid);
		void SetTextureUUIDPositiveBottom(const UUID& uuid);
		void SetTextureUUIDPositiveFront(const UUID& uuid);
		void SetTextureUUIDPositiveBack(const UUID& uuid);
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