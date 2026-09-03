#include "Renderer/Graph/Graph.hpp"
#include "Renderer/Graph/GraphTransientResourceCache.hpp"
#include "Renderer/Graph/Pass.hpp"
#include "Renderer/Graph/GraphTypes.hpp"
#include "Renderer/D3D12/D3D12GraphicsAPI.hpp"
#include "Renderer/D3D12/D3D12CommandList.hpp"
#include "Renderer/D3D12/D3D12CommandQueue.hpp"
#include "Renderer/D3D12/D3D12GraphicsCommands.hpp"

namespace Butterfly
{
	Graph::Graph(GraphTransientResourceCache& initializer)
		: ResourceInitializer(initializer)
	{
	}

	Graph::~Graph()
	{
		BF_PROFILE_EVENT()

		for (uint32_t i = 0; i < static_cast<uint32_t>(Passes.size()); i++)
		{
			delete Passes[i];
		}
	}

	void Graph::Execute(D3D12CommandList& cmdList) const
	{
		BF_PROFILE_EVENT();
		const uint32_t numPasses = static_cast<uint32_t>(Passes.size());

		uint32_t j = 0;
		for (uint32_t i = 0; i < numPasses; ++i)
		{
			cmdList.BeginGPUMarker("RenderPass -> " + Passes[i]->Name());
			GraphicsCommands::SetBindlessDescriptorHeapsAndRootSignature(cmdList);
			Passes[i]->Execute(cmdList);
			cmdList.EndGPUMarker();
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
