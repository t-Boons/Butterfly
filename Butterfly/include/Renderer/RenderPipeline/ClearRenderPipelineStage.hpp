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

		void SetClearColor(const glm::vec4& color) { m_clearColor = color; }

	private:
		glm::vec4 m_clearColor = { 0.05f, 0.05f, 0.1f, 1.0f };
	};
}
