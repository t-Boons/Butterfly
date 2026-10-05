#pragma once
#include "Renderer/D3D12Texture.hpp"
#include "Renderer/D3D12/D3D12Shader.hpp"
#include "Renderer/RenderPipeline/RenderPipeline.hpp"
#include "Renderer/Renderer.hpp"

namespace Butterfly
{
	class ViewportRenderEvent;
	class ClearRenderPipelineStage : public IRenderPipelineStage
	{
	public:
		virtual void OnRecordPass(const ViewportRenderEvent& ev) override;
		virtual void OnPostRender() override {}
	};
}
