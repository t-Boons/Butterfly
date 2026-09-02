#pragma once
#include "Core/Common.hpp"
#include "Renderer/D3D12/D3D12CommandList.hpp"

namespace Butterfly
{
	class D3D12CommandList;

	class PassBase
	{
	public:
		friend class GraphBuilder;
		friend class Graph;

		PassBase() = default;
		PassBase(const PassBase&) = delete;

		virtual ~PassBase() = default;

		const std::string& Name() const { return m_name; }

	protected:
		virtual void Execute(D3D12CommandList& ctx) = 0;

		const std::vector<std::type_index>& Dependencies() const { return m_dependencies; }

		std::vector<std::type_index> m_dependencies;
		std::string m_name;
	};

	template<typename Params>
	class Pass : public PassBase
	{
	public:
		friend class GraphBuilder;
		friend class Graph;

	protected:
		Pass();
		virtual void Execute(D3D12CommandList& cmdList) override;

	private:
		std::function<void(const Params&, D3D12CommandList&)> m_executeLambda;
		Params m_parameters;
	};

	////////////////////
	// Implementation //
	////////////////////

	template<typename Params>
	inline Pass<Params>::Pass()
		: m_parameters(Params())
	{
	}

	template<typename Params>
	inline void Pass<Params>::Execute(D3D12CommandList& cmdList)
	{
		BF_PROFILE_EVENT();

		m_executeLambda(m_parameters, cmdList);
	}

}
