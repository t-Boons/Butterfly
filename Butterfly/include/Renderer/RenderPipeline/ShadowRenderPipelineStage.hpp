#pragma once
#include "Renderer/RenderPipeline/RenderPipeline.hpp"

#define NUM_CASCADES 4

namespace Butterfly
{
	class BFTexture;
	struct ShadowMap
	{
		glm::mat4 LightViewProjections;
		BFTexture* ShadowMapTexture;
		float LightSize;
	};

	struct ShadowMapParameters
	{
		ShadowMap Map;
	};

	struct ShadowMaps
	{
		std::array<glm::mat4, NUM_CASCADES> LightViewProjections;
	};

	class ViewportRenderEvent;
	class ShadowRenderPipelineStage : public IRenderPipelineStage
	{
	public:
		virtual void OnRecordPass(const ViewportRenderEvent& ev) override;

		BFTexture* GetShadowMapTexture() const { return m_shadowMapTexture.get(); }
		ShadowMaps GetShadowMaps() const { return m_shadowMaps; }

	private:
		ShadowMaps m_shadowMaps;
		RefPtr<BFTexture> m_shadowMapTexture;
	};
}
