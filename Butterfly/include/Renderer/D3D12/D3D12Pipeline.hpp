#pragma once
#include "D3D12Common.hpp"

#include "Renderer/D3D12/D3D12Shader.hpp"

namespace Butterfly
{
	class BFShader;

	class DX12Pipeline : private NonCopyable
	{
	public:
		DX12Pipeline(const PipelineStateStream& pss);
		DX12Pipeline(const ComputePipelineStateStream& pss);
		~DX12Pipeline();

		ID3D12PipelineState* GetHW() const { return m_pso; }

	private:
		PipelineStateStream m_pss;
		ComputePipelineStateStream m_cpss;
		ID3D12PipelineState* m_pso;
	};

	class BFPipelineCache
	{
	public:
		static const DX12Pipeline& GetOrCreatePipeline(uint64_t hash, PipelineStateStream* pss);

	private:
		inline static std::unordered_map<size_t, const DX12Pipeline*> s_pipelines;
	};

	class BFComputePipelineState;

	class BFPipelineStateCache : private NonCopyableNonMoveable
	{
	public:
		static const DX12Pipeline& GetOrCreatePipeline(const BFComputePipelineState& state);

	private:
		inline static std::unordered_map<uint64_t, const DX12Pipeline*> s_pipelines;
	};

	class BFPipelineBuilder : private NonCopyable
	{
	public:
		BFPipelineBuilder()
			: m_hash(0u)
		{
		}

		BFPipelineBuilder& VertexShader(const BFShader* vs);
		BFPipelineBuilder& PixelShader(const BFShader* ps);

		BFPipelineBuilder& DepthStencilFormat(DXGI_FORMAT format);
		BFPipelineBuilder& CullingMode(D3D12_CULL_MODE mode);
		BFPipelineBuilder& RenderTargetFormats(const std::vector<DXGI_FORMAT>& formats);
		BFPipelineBuilder& PrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE type);
		BFPipelineBuilder& EnableBlending();

		BFPipelineBuilder& DepthEnable(bool enable);
		BFPipelineBuilder& DepthWriteMask(D3D12_DEPTH_WRITE_MASK mask);
		BFPipelineBuilder& DepthFunc(D3D12_COMPARISON_FUNC func);

		const DX12Pipeline& Create();

	private:
		PipelineStateStream m_pss = {};
		uint64_t m_hash;
	};



	struct RasterizerState
	{
		D3D12_CULL_MODE CullMode;
		bool EnableDepth = true;
	};

	struct BFGraphicsPipelineState
	{
		BFShader* VertexShader;
		BFShader* PixelShader;
		RasterizerState Rasterizer;
	};

	struct BFComputePipelineState
	{
		BFShader* ComputeShader;
	};
}

namespace std
{
	template<>
	struct hash<Butterfly::BFComputePipelineState>
	{
		size_t operator()(const Butterfly::BFComputePipelineState& state) const
		{
			size_t hash = state.ComputeShader->NumBytes();
			Butterfly::Utils::SumHash(hash, static_cast<uint64_t>(state.ComputeShader->Type()));;
			return hash;
		}
	};
}