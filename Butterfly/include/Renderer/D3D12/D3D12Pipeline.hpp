#pragma once
#include "D3D12Common.hpp"

#include "Renderer/D3D12/D3D12Shader.hpp"

namespace Butterfly
{
	class BFShader;
	class BFComputePSOInfo;
	class D3D12Pipeline : private NonCopyable
	{
	public:
		D3D12Pipeline(const PipelineStateStream& pss);
		D3D12Pipeline(const ComputePipelineStateStream& pss);
		~D3D12Pipeline();

		ID3D12PipelineState* GetHW() const { return m_pso; }

	private:
		PipelineStateStream m_pss;
		ComputePipelineStateStream m_cpss;
		ID3D12PipelineState* m_pso;
	};

	class BFPipelineCache
	{
	public:
		static const D3D12Pipeline& GetOrCreatePipeline(uint64_t hash, PipelineStateStream* pss);
		static const D3D12Pipeline& GetOrCreatePipeline(uint64_t hash, ComputePipelineStateStream* pss);
	private:
		inline static std::unordered_map<size_t, const D3D12Pipeline*> s_pipelines;
	};


	struct BlendState
	{
		bool EnableBlending = false;
		D3D12_BLEND SrcBlend = D3D12_BLEND_SRC_ALPHA;
		D3D12_BLEND DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		D3D12_BLEND_OP BlendOp = D3D12_BLEND_OP_ADD;

		D3D12_BLEND SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
		D3D12_BLEND DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
		D3D12_BLEND_OP BlendOpAlpha = D3D12_BLEND_OP_ADD;

		D3D12_COLOR_WRITE_ENABLE RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	};

	struct DepthStencilState
	{
		BlendState BlendStates[8];
		D3D12_COMPARISON_FUNC DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		D3D12_DEPTH_WRITE_MASK WriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		bool EnableDepth = true;
	};

	struct RasterizerState
	{
		D3D12_CULL_MODE CullMode = D3D12_CULL_MODE_BACK;
	};

	struct BFGraphicsPSOInfo
	{
		D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		BFShader* VertexShader;
		BFShader* PixelShader;
		RasterizerState Rasterizer;
		DepthStencilState DepthStencil;	
	};

	struct BFComputePSOInfo
	{
		BFShader* ComputeShader;
	};
}