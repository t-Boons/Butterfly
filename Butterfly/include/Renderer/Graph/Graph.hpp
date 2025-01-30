#pragma once
#include "Core/Common.hpp"

namespace Butterfly
{
	class PassBase;
	class GraphTransientResourceCache;

	class Graph : private NonCopyable
	{
	public:
		~Graph();

		void Execute() const;

		std::vector<const PassBase*> PassDependencies(const PassBase* pass) const;


		std::vector<PassBase*> SortedPasses;
		std::vector<uint32_t> PassFenceIndices;

		std::vector<PassBase*> Passes;
		std::unordered_map<std::type_index, int> PassTypes;

		GraphTransientResourceCache& ResourceInitializer;

	private:
		Graph(GraphTransientResourceCache& initializer);

		friend class GraphBuilder;
	};
}