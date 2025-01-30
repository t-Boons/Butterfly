#include "Renderer/D3D12/Pipeline_DX12.hpp"
#include "Renderer/D3D12/Shader_DX12.hpp"
#include "Renderer/D3D12/GraphicsAPI_DX12.hpp"

namespace Butterfly
{
	DX12Pipeline::DX12Pipeline(const PipelineStateStream& pss)
		: m_pss(pss)
	{
		BF_PROFILE_EVENT();

		D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = { sizeof(PipelineStateStream), &m_pss };

		ThrowIfFailed(DX12API()->Device()->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_pso)));
	}

	DX12Pipeline::~DX12Pipeline()
	{
		COM_FREE(m_pso);
	}


	const DX12Pipeline& BFPipelineCache::GetOrCreatePipeline(uint64_t hash, PipelineStateStream* pss)
	{
		BF_PROFILE_EVENT();

		auto pso = s_pipelines.find(hash);
		if (pso == s_pipelines.end())
		{
			Log::Info("Created New PSO with hash: %llu", hash);
			s_pipelines[hash] = new DX12Pipeline(*pss);
		}
		return *s_pipelines[hash];
	}


	BFPipelineBuilder& BFPipelineBuilder::VertexShader(const BFShader* vs)
	{
		BF_PROFILE_EVENT();

		Check(vs);
		CheckMsg(vs->Type() == ShaderType::Vertex, "PipelineBuilder shader is not a Vertex shader.");
		m_pss.VS = { vs->Blob()->GetBufferPointer(), vs->Blob()->GetBufferSize() };
		Utils::SumHash(m_hash, static_cast<uint64_t>(vs->NumBytes()));
		return *this;
	}

	BFPipelineBuilder& BFPipelineBuilder::PixelShader(const BFShader* ps)
	{
		BF_PROFILE_EVENT();

		Check(ps);
		CheckMsg(ps->Type() == ShaderType::Pixel, "PipelineBuilder shader is not a Pixel shader.");
		m_pss.PS = { ps->Blob()->GetBufferPointer(), ps->Blob()->GetBufferSize() };
		Utils::SumHash(m_hash, static_cast<uint64_t>(ps->NumBytes()));
		return *this;
	}


	BFPipelineBuilder& BFPipelineBuilder::DepthStencilFormat(DXGI_FORMAT format)
	{
		BF_PROFILE_EVENT();

		m_pss.DSVFormat = format;
		Utils::SumHash(m_hash, static_cast<uint64_t>(format));
		return *this;
	}

	BFPipelineBuilder& BFPipelineBuilder::CullingMode(D3D12_CULL_MODE mode)
	{
		BF_PROFILE_EVENT();

		CD3DX12_RASTERIZER_DESC rasterizerDesc(m_pss.RasterizerState);
		rasterizerDesc.CullMode = mode;
		m_pss.RasterizerState = rasterizerDesc;
		Utils::SumHash(m_hash, static_cast<uint64_t>(mode));
		return *this;
	}

	BFPipelineBuilder& BFPipelineBuilder::RenderTargetFormats(const std::vector<DXGI_FORMAT>& formats)
	{
		BF_PROFILE_EVENT();

		CheckMsg(formats.size() < 8, "RenderTarget format count exceeds 8, currently: %lu", formats.size())

			D3D12_RT_FORMAT_ARRAY rtvFormats = {};
		rtvFormats.NumRenderTargets = static_cast<uint32_t>(formats.size());
		for (size_t i = 0; i < formats.size(); i++)
		{
			rtvFormats.RTFormats[i] = formats[i];
			Utils::SumHash(m_hash, static_cast<uint64_t>(formats[i]));
		}
		m_pss.RTVFormats = rtvFormats;

		return *this;
	}

	BFPipelineBuilder& BFPipelineBuilder::PrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE type)
	{
		BF_PROFILE_EVENT();

		m_pss.PrimitiveTopologyType = type;
		Utils::SumHash(m_hash, static_cast<uint64_t>(type));
		return *this;
	}

	BFPipelineBuilder& BFPipelineBuilder::EnableBlending()
	{
		CD3DX12_BLEND_DESC blendDesc(m_pss.BlendDesc);

		D3D12_RENDER_TARGET_BLEND_DESC rtBlendDesc = {};
		rtBlendDesc.BlendEnable = TRUE;
		rtBlendDesc.LogicOpEnable = FALSE;
		rtBlendDesc.SrcBlend = D3D12_BLEND_ONE;
		rtBlendDesc.DestBlend = D3D12_BLEND_ONE;
		rtBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
		rtBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE; 
		rtBlendDesc.DestBlendAlpha = D3D12_BLEND_ONE; 
		rtBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		rtBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		blendDesc.RenderTarget[0] = rtBlendDesc;

		m_pss.BlendDesc = blendDesc;
		Utils::SumHash(m_hash, static_cast<uint64_t>(237589327));
		return *this;
	}

	const DX12Pipeline& BFPipelineBuilder::Create()
	{
		BF_PROFILE_EVENT();

		m_pss.pRootSignature = DX12API()->BindlessRootSignature();

		return BFPipelineCache::GetOrCreatePipeline(m_hash, &m_pss);
	}
}