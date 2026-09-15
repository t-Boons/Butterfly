#pragma once
#include "Core/Common.hpp"
#include "Asset/AssetTypes.hpp"
#include "Core/Application.hpp"
#include "Asset/AssetManager.hpp"

namespace Butterfly
{
	class MeshRendererComponent : public NonCopyable
	{
	public:
		void SetMeshHandle(const AssetHandle<MeshAsset>& handle)
		{
			m_meshHandle = handle;
			m_serializeID = handle.GetID();
		}

		const AssetHandle<MeshAsset>& GetMeshHandle() const
		{
			return m_meshHandle;
		}

	private:
		void SetSerializeID(const UUID& id)
		{
			m_serializeID = id;
			Application::Get().GetAssetManager().Acquire(id, m_meshHandle);
		}
		const UUID& GetSerializeID() const
		{
			return m_serializeID;
		}

		AssetHandle<MeshAsset> m_meshHandle;
		UUID m_serializeID;
		friend class ComponentRegistry;
	};
}
