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
	class BFTexture;
	struct ShadowMapParameters
	{
		glm::mat4 LightViewProjection;
		BFTexture* Shadowmap;
		float LightSize;
	};


	class ViewportRenderEvent;
	class ShadowRenderPipelineStage : public IRenderPipelineStage
	{
	public:
		virtual void OnRecordPass(const ViewportRenderEvent& ev) override;

		void Update(const SkyboxComponent* component);
	private:
		void LoadSkybox(const SkyboxComponent& component);
		void UnloadSkybox();
		bool IsSkyboxLoaded() const { return m_skyboxTexture != nullptr; }
		RefPtr<BFTexture> m_skyboxTexture;
		int m_skyboxVersion = -1;
	};
}
