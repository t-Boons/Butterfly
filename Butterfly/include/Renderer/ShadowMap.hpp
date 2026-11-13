#pragma once
#include "Core/Common.hpp"
#include "Scene/Registry/LightComponent.hpp"
#include "Renderer/RenderIncludes.hpp"

#define MAX_SHADOW_MAPS 16

namespace Butterfly
{

	struct GPUShadowMap
	{
		std::array<glm::mat4, CUBEMAP_SLICES> LightViewProjections;
		std::array<float, MAX_CASCADES> CascadeSplits;
		std::array<float, MAX_CASCADES> TexelSizes;
		int ShadowMapTextureIndex = -1;
		int NumSlices = -1;
		float Resolution = 0.0f;
	};

	class BFTexture;
	struct ShadowMap
	{
		std::array<glm::mat4, CUBEMAP_SLICES> LightViewProjections;
		std::array<float, MAX_CASCADES> CascadeSplits;
		std::array<float, MAX_CASCADES> TexelSizes;
		RefPtr<BFTexture> ShadowMapTexture;
		int NumSlices = -1;
		float Resolution = 0.0f;
	};


	class Viewport;
	class BFStructuredBuffer;
	class BFShaderResourceView;
	class ShadowMapBuffer
	{
	public:
		ShadowMapBuffer();

		void Update(const Viewport& viewport);

		const BFShaderResourceView& SRV() const;
		const std::array<ShadowMap, MAX_SHADOW_MAPS>& GetShadowMaps() const { return m_shadowMaps; }
		uint32_t GetNumShadowMaps() const { return static_cast<uint32_t>(m_shadowMaps.size()); }

	private:
		RefPtr<BFStructuredBuffer> m_gpuShadowMapsBuffer;
		std::array<GPUShadowMap, MAX_SHADOW_MAPS> m_gpuShadowMaps;
		std::array<ShadowMap, MAX_SHADOW_MAPS> m_shadowMaps;
	};
}

