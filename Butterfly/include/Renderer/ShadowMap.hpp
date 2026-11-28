#pragma once
#include "Core/Common.hpp"
#include "Level/Registry/LightComponent.hpp"
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

	class Texture;
	struct ShadowMap
	{
		std::array<glm::mat4, CUBEMAP_SLICES> LightViewProjections;
		std::array<float, MAX_CASCADES> CascadeSplits;
		std::array<float, MAX_CASCADES> TexelSizes;
		RefPtr<Texture> ShadowMapTexture;
		int NumSlices = -1;
		float Resolution = 0.0f;
	};


	class Viewport;
	class StructuredBuffer;
	class ShaderResourceView;
	class ShadowMapBuffer
	{
	public:
		ShadowMapBuffer();

		void Update(const Viewport& viewport);

		const ShaderResourceView& SRV() const;
		const std::array<ShadowMap, MAX_SHADOW_MAPS>& GetShadowMaps() const { return m_shadowMaps; }
		uint32_t GetNumShadowMaps() const { return static_cast<uint32_t>(m_shadowMaps.size()); }

	private:
		RefPtr<StructuredBuffer> m_gpuShadowMapsBuffer;
		std::array<GPUShadowMap, MAX_SHADOW_MAPS> m_gpuShadowMaps;
		std::array<ShadowMap, MAX_SHADOW_MAPS> m_shadowMaps;
	};
}
