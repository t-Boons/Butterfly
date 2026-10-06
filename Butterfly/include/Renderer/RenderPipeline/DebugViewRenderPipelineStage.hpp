#pragma once
#include "Renderer/RenderPipeline/RenderPipeline.hpp"

namespace Butterfly
{
	class ViewportRenderEvent;


	enum class DebugViewType
	{
		None,
		Normal,
		Albedo,
		Roughness,
		Metallic,
		Emission,
		Depth,
		UVs,
	};

	class DebugViewRenderPipelineStage : public IRenderPipelineStage
	{
	public:
		virtual void OnRecordPass(const ViewportRenderEvent& ev) override;
		virtual void OnPostRender() override {}

		void SetDebugViewType(DebugViewType type) { m_debugViewType = type; }
	private:
		DebugViewType m_debugViewType = DebugViewType::None;
	};
}