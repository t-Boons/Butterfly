#pragma once
#include "Renderer/RenderPipeline/RenderPipeline.hpp"

namespace Butterfly
{
	class ViewportRenderEvent;
	class ColorspaceCorrectionRenderPipelineStage : public IRenderPipelineStage
	{
	public:
		enum class ColorSpace : uint32_t
		{
			SRGB,
			GammaApprox,
			Linear
		};

		virtual void OnRecordPass(const ViewportRenderEvent& ev) override;

		void SetColorSpace(ColorSpace colorSpace) { m_colorSpace = colorSpace; }

	private:
		ColorSpace m_colorSpace = ColorSpace::SRGB;
	};
}