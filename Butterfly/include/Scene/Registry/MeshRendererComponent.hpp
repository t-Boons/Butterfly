#pragma once
#include "Core/Common.hpp"
#include "Asset/AssetTypes.hpp"
#include "Core/Application.hpp"
#include "Asset/AssetManager.hpp"

namespace Butterfly
{
	class MeshRendererComponent
	{
	public:
		MeshRendererComponent(const Entity& entity)
		{
		}

		void SetMeshHandle(const AssetHandle<MeshAsset>& handle)
		{
			m_meshHandle = handle;
			m_meshUUID = handle.GetID();
		}

		const AssetHandle<MeshAsset>& GetMeshHandle() const
		{
			return m_meshHandle;
		}

	private:
		AssetHandle<MeshAsset>& GetMeshHandleRef()
		{
			return m_meshHandle;
		}

		void SetMeshUUID(const AssetUUID<MeshAsset>& id)
		{
			if(Application::Get().GetAssetManager().Acquire(id, m_meshHandle))
			{
				m_meshUUID = id;
			}
		}
		const AssetUUID<MeshAsset>& GetMeshUUID() const
		{
			return m_meshUUID;
		}

		AssetHandle<MeshAsset> m_meshHandle;
		AssetUUID<MeshAsset> m_meshUUID;
		friend class ComponentRegistry;
	};
}
