#pragma once
#include "Renderer/D3D12Texture.hpp"
#include "Scene/Registry/SkyboxComponent.hpp"
#include "Renderer/D3D12/D3D12GraphicsCommands.hpp"
#include "Renderer/D3D12/D3D12Shader.hpp"
#include "Renderer/RenderPipeline/RenderPipeline.hpp"
#include "Renderer/Renderer.hpp"


namespace Butterfly
{
	class Texture;

	struct EnviromentMap
	{
		RefPtr<Texture> Enviroment;
		RefPtr<Texture> Irradiance;
		RefPtr<Texture> Prefiltered;
	};

	struct SkyboxPassParams
	{
		Texture* SkyboxTexture;
		Texture* IrradianceTexture;
	};


	class ViewportRenderEvent;
	class SkyboxRenderPipelineStage : public IRenderPipelineStage
	{
	public:
		virtual void OnRecordPass(const ViewportRenderEvent& ev) override;

		void Update(const SkyboxComponent* component);
	private:
		void LoadSkybox(const SkyboxComponent& component);
		void UnloadSkybox();
		bool IsSkyboxLoaded() const { return m_skyboxTexture != nullptr; }
		RefPtr<Texture> m_skyboxTexture;
		RefPtr<Texture> m_irradianceTexture;
		int m_skyboxVersion = -1;
	};
}
