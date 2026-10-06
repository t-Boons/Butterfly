#include "Renderer/RenderPipeline/RenderPipeline.hpp"
#include "Renderer/RenderPipeline/PBRRenderPipelineStage.hpp"
#include "Renderer/RenderPipeline/SkyboxRenderPipelineStage.hpp"
#include "Renderer/RenderPipeline/ColorspaceCorrectionRenderPipelineStage.hpp"
#include "Renderer/RenderPipeline/ClearRenderPipelineStage.hpp"

namespace Butterfly
{
	RenderPipeline::RenderPipeline(Renderer& renderer)
		: m_renderer(renderer)
	{
		AddDefaultStages();
	}

	void RenderPipeline::RecordPasses(const ViewportRenderEvent& ev)
	{
		for (auto& stage : m_renderPipelineStages)
		{
			stage->OnRecordPass(ev);
		}
	}

	void RenderPipeline::PostRender()
	{
		for (auto& stage : m_renderPipelineStages)
		{
			stage->OnPostRender();
		}
	}

	void RenderPipeline::ClearStages()
	{
		m_renderPipelineStages.clear();
	}

	void RenderPipeline::AddDefaultStages()
	{
		ClearStages();
		RegisterStage<ClearRenderPipelineStage>();
		RegisterStage<SkyboxRenderPipelineStage>();
		RegisterStage<PBRRenderPipelineStage>();
		RegisterStage<ColorspaceCorrectionRenderPipelineStage>();
	}

}