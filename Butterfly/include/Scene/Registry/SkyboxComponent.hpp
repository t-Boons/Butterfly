#pragma once
#include "Core/Common.hpp"
#include "Asset/AssetTypes.hpp"
#include "Asset/AssetManager.hpp"

namespace Butterfly
{
	enum class SkyboxType : uint32_t
	{
		Equirectangular,
		Cubemap
	};


	class Entity;
	class SkyboxComponent
	{
	public:
		SkyboxComponent(const Entity& entity);
		void SetTextureHandle(uint32_t side, const AssetHandle<TextureAsset>& handle);
		void SetTextureHandleHDRI(const AssetHandle<TextureAsset>& handle);
		const AssetHandle<TextureAsset>& GetTextureHandle(uint32_t side) const;
		const AssetHandle<TextureAsset>& GetTextureHandleHDRI() const { return m_hdriTexture; }
		bool IsDirty() const { return m_isDirty; }
		void ClearDirty() { m_isDirty = false; }

		SkyboxType GetType() const { return static_cast<SkyboxType>(m_type); }
		void SetType(SkyboxType type);

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

		void SetTextureUUIDHDRI(const UUID& uuid);
		const UUID& GetTextureUUIDHDRI() const { return m_hdriUUID; }

		void SetTypeUInt(uint32_t type) { m_type = type; MarkDirty(); }
		uint32_t GetTypeUInt() const { return m_type; }

		friend class ComponentRegistry;

		uint32_t m_type = static_cast<uint32_t>(SkyboxType::Equirectangular);
		UUID m_hdriUUID;
		AssetHandle<TextureAsset> m_hdriTexture;

		std::array<UUID, 6> m_serializeUUIDs;
		std::array<AssetHandle<TextureAsset>, 6> m_textureHandles;
		bool m_isDirty = true;
	};
}