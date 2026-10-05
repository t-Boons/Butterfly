#include "Renderer/D3D12/D3D12Pipeline.hpp"
#include "Renderer/D3D12/D3D12Shader.hpp"
#include "Renderer/D3D12/D3D12GraphicsAPI.hpp"

namespace Butterfly
{
	D3D12Pipeline::D3D12Pipeline(const PipelineStateStream& pss)
		: m_pss(pss)
	{
		BF_PROFILE_EVENT();

		D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = { sizeof(PipelineStateStream), &m_pss };

		ThrowIfFailed(D3D12API()->Device()->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_pso)));
	}

	D3D12Pipeline::D3D12Pipeline(const ComputePipelineStateStream& pss)
		: m_cpss(pss)
	{
		BF_PROFILE_EVENT();
		D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = { sizeof(ComputePipelineStateStream), &m_cpss };
		ThrowIfFailed(D3D12API()->Device()->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_pso)));
	}

	D3D12Pipeline::~D3D12Pipeline()
	{
		COM_FREE(m_pso);
	}


	const D3D12Pipeline& BFPipelineCache::GetOrCreatePipeline(uint64_t hash, PipelineStateStream* pss)
	{
		BF_PROFILE_EVENT();

		auto pso = s_pipelines.find(hash);
		if (pso == s_pipelines.end())
		{
			BF_CORE_LOG_INFO("Created New PSO with hash: %llu", hash);
			s_pipelines[hash] = new D3D12Pipeline(*pss);
		}
		return *s_pipelines[hash];
	}
	
	const D3D12Pipeline& BFPipelineCache::GetOrCreatePipeline(uint64_t hash, ComputePipelineStateStream* pss)
	{
		BF_PROFILE_EVENT();

		auto pso = s_pipelines.find(hash);
		if (pso == s_pipelines.end())
		{
			BF_CORE_LOG_INFO("Created New PSO with hash: %llu", hash);
			s_pipelines[hash] = new D3D12Pipeline(*pss);
		}
		return *s_pipelines[hash];
	}
}
