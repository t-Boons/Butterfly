#pragma once
#include "Renderer/D3D12Texture.hpp"
#include "stbimage/stb_image.h"
#include "Scene/Registry/SkyboxComponent.hpp"
#include "Renderer/Graph/GraphBuilder.hpp"
#include "Renderer/D3D12/D3D12Pipeline.hpp"

#include "Renderer/D3D12/D3D12GraphicsCommands.hpp"
#include "Renderer/D3D12/D3D12Shader.hpp"
#include "Renderer/RenderPipeline/RenderPipeline.hpp"
#include "Renderer/Renderer.hpp"


namespace Butterfly
{
	class ViewportRenderEvent;
	class SkyboxRenderPipelineStage : public IRenderPipelineStage
	{
	public:
		virtual void OnRecordPass(const ViewportRenderEvent& ev) override;
		virtual void OnPostRender() override {}

		void LoadSkybox(const SkyboxComponent& component);
		void UnloadSkybox();
		bool IsSkyboxLoaded() const { return m_skyboxTexture != nullptr; }
	private:

		RefPtr<BFTexture> m_skyboxTexture;
	};
}
