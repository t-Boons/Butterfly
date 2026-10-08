#pragma once
#include "Renderer/RenderPipeline/RenderPipeline.hpp"

namespace Butterfly
{
	class ViewportRenderEvent;
	class PBRRenderPipelineStage : public IRenderPipelineStage
	{
	public:
		PBRRenderPipelineStage();
		virtual void OnRecordPass(const ViewportRenderEvent& ev) override;

	private:
		RefPtr<BFTexture> m_whiteTexture;
		RefPtr<BFSampler> m_defaultSampler;
	};
}