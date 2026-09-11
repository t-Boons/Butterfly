#pragma once
#include "Core/Common.hpp"
#include "Renderer/D3D12/D3D12Common.hpp"
#include "Renderer/D3D12Texture.hpp"
#include "Renderer/D3d12Buffer.hpp"
#include "Renderer/Graph/Graph.hpp"
#include "Renderer/D3D12/D3D12Fence.hpp"
#include "Renderer/Graph/Blackboard.hpp"
#include "Core/Window.hpp"
#include "Renderer/ModelLoading/ModelImporter.hpp"
#include "Core/Application.hpp"

#include "Renderer/GraphicsAPI.hpp"
#include "Renderer/D3D12/D3D12GraphicsAPI.hpp"
#include "Renderer/D3D12/D3D12DescriptorAllocator.hpp"
#include "Renderer/D3D12/D3D12CommandQueue.hpp"
#include "Renderer/D3D12/D3D12CommandList.hpp"
#include "Renderer/D3D12/D3D12GraphicsCommands.hpp"
#include "Renderer/D3D12/D3D12Shader.hpp"
#include "Renderer/D3D12/D3D12ShaderVariables.hpp"
#include "Renderer/D3D12Sampler.hpp"
#include "Renderer/D3D12/D3D12Pipeline.hpp"
#include "Renderer/Graph/GraphBuilder.hpp"
#include "Renderer/D3D12/D3D12View.hpp"
#include "Core/JobSystem.hpp"

#include "Asset/AssetTypes.hpp"

namespace Butterfly
{
	class MeshRendererComponent : public NonCopyable
	{
	public:
		bool ContainsMesh() const
		{
			return MeshHandle.Valid();
		}

		AssetHandle<MeshAsset> MeshHandle;
	};
}
