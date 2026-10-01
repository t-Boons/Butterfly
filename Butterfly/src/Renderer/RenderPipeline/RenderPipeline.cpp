#include "Renderer/RenderPipeline/RenderPipeline.hpp"
#include "Renderer/RenderPipeline/PBRRenderPipelineStage.hpp"
#include "Renderer/RenderPipeline/SkyboxRenderPipelineStage.hpp"
#include "Renderer/RenderPipeline/ColorspaceCorrectionRenderPipelineStage.hpp"
namespace Butterfly
{
	RenderPipeline::RenderPipeline(Renderer& renderer)
		: m_renderer(renderer)
	{
		RegisterStage<PBRRenderPipelineStage>();
		RegisterStage<SkyboxRenderPipelineStage>();
		RegisterStage<ColorspaceCorrectionRenderPipelineStage>();
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

}