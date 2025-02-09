#pragma once
#include "Core/Common.hpp"
#include "Pass.hpp"
#include "Graph.hpp"
#include "GraphTypes.hpp"
#include "GraphTransientResourceCache.hpp"

namespace Butterfly
{
	class Graph;
	class BFRGTexture;
	struct BFRGTextureInitializer;

	class GraphBuilder : private NonCopyable
	{
	public:
		GraphBuilder(GraphTransientResourceCache& initializer);
		~GraphBuilder();

		template<typename PassType, typename ParamStruct>
		ParamStruct* GetPassData() const;

		template<typename ParamStruct>
		void AddPass(const std::string& name, std::function<void(const ParamStruct&, DX12CommandList&)>&& exec);

		template<typename ParamStruct>
		[[nodiscard]] ParamStruct* AllocParameters();
			
		template<typename Args>
		[[nodiscard]] BFRGTexture* CreateTransientTexture(const std::string& key, Args&& args);

		template<typename ParamStruct>
		bool HasPassType() const;

		const Graph* Create();

	private:
		void SortPasses();
		void CalculateFencePoints();

		template<typename ParamStruct>
		Pass<ParamStruct>* PassFromType() const;


		Graph* m_graph;
		bool m_hasBeenCreated;
	};

	////////////////////
	// Implementation //
	////////////////////

	inline GraphBuilder::GraphBuilder(GraphTransientResourceCache& initializer)
		: m_graph(new Graph(initializer)), m_hasBeenCreated(false)
	{
	}

	inline GraphBuilder::~GraphBuilder()
	{
		BF_PROFILE_EVENT();

		if (!m_hasBeenCreated)
		{
			Log::Warn("GraphBuilder instance has been deleted without a graph being made.");
			delete m_graph;
		}
	}

	template<typename Args>
	inline BFRGTexture* GraphBuilder::CreateTransientTexture(const std::string& key, Args&& args)
	{
		return &m_graph->ResourceInitializer.GetOrCreateTexture<BFRGTexture>(key, args);
	}


	template<typename PassType, typename ParamStruct>
	inline ParamStruct* GraphBuilder::GetPassData() const
	{
		BF_PROFILE_EVENT();

		static_assert(!std::is_same<PassType, ParamStruct>::value, "PassType and ParamStruct may not be the same type.");
		CheckMsg(HasPassType<ParamStruct>(), "GraphBuilder does not have a pass with type: %s", typeid(ParamStruct).name());


		auto& depVec = PassFromType<PassType>()->m_dependencies;
		if (std::find(depVec.begin(), depVec.end(), typeid(ParamStruct)) == depVec.end())
		{
			depVec.push_back(typeid(ParamStruct));
		}

		Pass<ParamStruct>* pass = PassFromType<ParamStruct>();
		return &pass->m_parameters;
	}


	template<typename ParamStruct>
	inline void GraphBuilder::AddPass(const std::string& name, std::function<void(const ParamStruct&, DX12CommandList&)>&& exec)
	{
		BF_PROFILE_EVENT();

		PassFromType<ParamStruct>()->m_executeLambda = exec;
		PassFromType<ParamStruct>()->m_name = name;
	}

	template<typename ParamStruct>
	inline ParamStruct* GraphBuilder::AllocParameters()
	{
		BF_PROFILE_EVENT();

		CheckMsg(!HasPassType<ParamStruct>(), "Duplicate pass with ParamStruct: %s", typeid(ParamStruct).name());
		auto* pass = new Pass<ParamStruct>();

		m_graph->PassTypes[typeid(ParamStruct)] = static_cast<uint32_t>(m_graph->Passes.size());
		m_graph->Passes.push_back(pass);

		return &pass->m_parameters;
	}

	template<typename ParamStruct>
	inline Pass<ParamStruct>* GraphBuilder::PassFromType() const
	{
		BF_PROFILE_EVENT();

		CheckMsg(HasPassType<ParamStruct>(), "Pass with type ParamStruct cannot be found: %s", typeid(ParamStruct).name());
		return reinterpret_cast<Pass<ParamStruct>*>(m_graph->Passes[m_graph->PassTypes[typeid(ParamStruct)]]);
	}

	template<typename ParamStruct>
	inline bool GraphBuilder::HasPassType() const
	{
		BF_PROFILE_EVENT();

		return m_graph->PassTypes.find(typeid(ParamStruct)) != m_graph->PassTypes.end();
	}

	inline const Graph* GraphBuilder::Create()
	{
		BF_PROFILE_EVENT();

		SortPasses();
		CalculateFencePoints();

		m_hasBeenCreated = true;
		return m_graph;
	}

	inline void GraphBuilder::SortPasses()
	{
		BF_PROFILE_EVENT();

		const uint32_t numPasses = static_cast<uint32_t>(m_graph->Passes.size());

		// Prepare for topological sort.
		std::vector<std::vector<int>> passes;
		passes.reserve(numPasses);
		for (int i = 0; i < static_cast<int>(numPasses); i++)
		{
			if (m_graph->Passes[i]->m_dependencies.empty()) continue;
			std::vector<int> deps;
			for (int j = 0; j < m_graph->Passes[i]->m_dependencies.size(); j++)
			{
				passes.push_back({ m_graph->PassTypes[m_graph->Passes[i]->m_dependencies[j]], i });
			}
		}

		// Do topological sort.
		auto sortedIndices = Utils::TopologicalSortKahn(numPasses, passes);

		m_graph->SortedPasses.reserve(numPasses);
		for (uint32_t i = 0; i < numPasses; i++)
		{
			m_graph->SortedPasses.push_back(m_graph->Passes[sortedIndices[i]]);
		}
	}

	inline void GraphBuilder::CalculateFencePoints()
	{
		BF_PROFILE_EVENT();

		// Record fence points;
		const uint32_t numPasses = static_cast<uint32_t>(m_graph->SortedPasses.size());
		for (uint32_t i = 0; i < numPasses; i++)
		{
			if (!m_graph->SortedPasses[i]->m_dependencies.empty())
			{
				m_graph->PassFenceIndices.push_back(i - 1);
			}
		}

		// Add a fence point at the end so the graph since it is always neccesary.
		m_graph->PassFenceIndices.push_back(numPasses - 1);
	}
}