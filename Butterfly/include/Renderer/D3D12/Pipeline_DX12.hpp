#pragma once
#include "Common_DX12.hpp"

namespace Butterfly
{
	class BFShader;

	class DX12Pipeline : private NonCopyable
	{
	public:
		DX12Pipeline(const PipelineStateStream& pss);
		~DX12Pipeline();

		ID3D12PipelineState* GetHW() const { return m_pso; }

	private:
		PipelineStateStream m_pss;
		ID3D12PipelineState* m_pso;
	};

	class BFPipelineCache
	{
	public:
		static const DX12Pipeline& GetOrCreatePipeline(uint64_t hash, PipelineStateStream* pss);

	private:
		inline static std::unordered_map<size_t, const DX12Pipeline*> s_pipelines;
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

		const DX12Pipeline& Create();

	private:
		PipelineStateStream m_pss = {};
		uint64_t m_hash;
	};
}