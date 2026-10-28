#pragma once
#include "Renderer/RenderPipeline/RenderPipeline.hpp"

namespace Butterfly
{
	class BFStructuredBuffer;
	struct ShadowMapPass
	{
	};

	class ViewportRenderEvent;
	class ShadowRenderPipelineStage : public IRenderPipelineStage
	{
	public:
		virtual void OnRecordPass(const ViewportRenderEvent& ev) override;
	};
}
