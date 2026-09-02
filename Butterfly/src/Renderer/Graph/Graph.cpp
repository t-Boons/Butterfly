#include "Renderer/Graph/Graph.hpp"
#include "Renderer/Graph/GraphTransientResourceCache.hpp"
#include "Renderer/Graph/Pass.hpp"
#include "Renderer/Graph/GraphTypes.hpp"
#include "Renderer/D3D12/GraphicsAPI_DX12.hpp"
#include "Renderer/D3D12/CommandList_DX12.hpp"
#include "Renderer/D3D12/CommandQueue_DX12.hpp"
#include "Renderer/D3D12/GraphicsCommands_DX12.hpp"

namespace Butterfly
{
	Graph::Graph(GraphTransientResourceCache& initializer)
		: ResourceInitializer(initializer)
	{
	}

	Graph::~Graph()
	{
		BF_PROFILE_EVENT()

		for (uint32_t i = 0; i < static_cast<uint32_t>(SortedPasses.size()); i++)
		{
			delete SortedPasses[i];
		}
		SortedPasses.clear();
	}

	void Graph::Execute() const
	{
		BF_PROFILE_EVENT();
		const uint32_t numPasses = static_cast<uint32_t>(SortedPasses.size());

		const auto cache = CommandListCache();
		std::vector<DX12CommandListHandle*> handles;
		handles.reserve(numPasses);

		for (uint32_t i = 0; i < numPasses; i++)
		{
			auto newList = cache->GetNewList();
			handles.push_back(newList);
		}
		
		std::vector<ID3D12CommandList*> cmdListQueue;
		cmdListQueue.reserve(numPasses);
		
		uint32_t j = 0;
		for (uint32_t i = 0; i < numPasses; ++i)
		{
			DX12CommandList& list = handles[i]->List();
			list.BeginGPUMarker("RenderPass -> " + SortedPasses[i]->Name());
			GraphicsCommands::SetBindlessDescriptorHeapsAndRootSignature(list);
			SortedPasses[i]->Execute(list);
			list.EndGPUMarker();
			list.Close();

			cmdListQueue.push_back(list.List());

			if (i == PassFenceIndices[j])
			{
				DX12API()->Queue(QueueType::Direct)->Execute(static_cast<uint32_t>(cmdListQueue.size()), cmdListQueue.data());

				// Wait till previous work is executed.
				DX12API()->Queue(QueueType::Direct)->WaitForFence();

				cmdListQueue.clear();
				j++;
			}
		}

		DX12API()->Queue(QueueType::Direct)->WaitForFence();

		for (uint32_t i = 0; i < numPasses; ++i)
		{
			handles[i]->List().Reset();
			delete handles[i];
		}

		ResourceInitializer.UpdateLifetimes();
	}

	std::vector<const PassBase*> Graph::PassDependencies(const PassBase* pass) const
	{
		BF_PROFILE_EVENT()

		std::vector<const PassBase*> out;
		const std::vector<std::type_index>& ids = pass->Dependencies();
		const uint32_t numDeps = static_cast<uint32_t>(ids.size());
		out.reserve(numDeps);
	
		for (uint32_t i = 0; i < numDeps; i++)
		{
			const std::type_index id = ids[i];
			out.push_back(Passes[PassTypes.at(id)]);
		}

		return out;
	}
}