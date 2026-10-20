#pragma once
#include "Renderer/RenderPipeline/RenderPipeline.hpp"

#define NUM_CASCADES 4

namespace Butterfly
{
	class BFStructuredBuffer;
	struct ShadowMapPass
	{
		BFStructuredBuffer* ShadowMapBuffer;
		uint32_t NumShadowMaps;
	};

	struct GPUShadowMap
	{
		glm::mat4 LightViewProjections[NUM_CASCADES];
		float CascadeSplits[NUM_CASCADES];
		int ShadowMapTextureIndex;
		int NumCascades;
		float Size;
	};

	class BFTexture;
	struct ShadowMap
	{
		glm::mat4 LightViewProjections[NUM_CASCADES];
		float CascadeSplits[NUM_CASCADES];
		BFTexture* ShadowMapTexture;
		int NumCascades;
		float Size;
	};

	class ViewportRenderEvent;
	class ShadowRenderPipelineStage : public IRenderPipelineStage
	{
	public:
		virtual void OnRecordPass(const ViewportRenderEvent& ev) override;

		const std::vector<ShadowMap>& GetShadowMaps() const { return m_shadowMaps; }

	private:
		std::vector<ShadowMap> m_shadowMaps;
	};
}
