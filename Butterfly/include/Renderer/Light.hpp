#pragma once
#include "Core/Common.hpp"
#include "Scene/Registry/LightComponent.hpp"

namespace Butterfly
{
	class BFStructuredBuffer;
	class BFTexture;

	struct GPUShadowMap
	{
		std::array<glm::mat4, CUBEMAP_SLICES> LightViewProjections;
		std::array<float, MAX_CASCADES> CascadeSplits;
		std::array<float, MAX_CASCADES> TexelSizes;
		int ShadowMapTextureIndex = -1;
		int NumSlices = -1;
		float Resolution = 0.0f;
	};

	struct GPULight
	{
		uint32_t Type;
		glm::vec3 Color;
		glm::vec3 Position;
		glm::vec3 Direction;
		float Range;
		float InnerConeAngleCos;
		float OuterConeAngleCos;
		GPUShadowMap ShadowMap;
	};

	struct ShadowMap
	{
		std::array<glm::mat4, CUBEMAP_SLICES> LightViewProjections;
		std::array<float, MAX_CASCADES> CascadeSplits;
		std::array<float, MAX_CASCADES> TexelSizes;
		BFTexture* ShadowMapTexture;
		int NumSlices = -1;
		float Resolution = 0.0f;
	};

	struct Light
	{
		LightType Type;
		glm::vec3 Color;
		glm::vec3 Position;
		glm::vec3 Direction;
		float Range;
		float InnerConeAngleCos;
		float OuterConeAngleCos;
		ShadowMap ShadowMap;

		GPULight ToGPULight() const;
	};

	class Viewport;
	class BFShaderResourceView;
	class LightBuffer
	{
	public:
		LightBuffer();

		void Update(const Viewport& viewport);

		const std::vector<Light>& GetLights() const;
		const BFShaderResourceView& SRV() const;

	private:
		RefPtr<BFStructuredBuffer> m_gpuLights;
		std::vector<GPULight> m_gpuLightsVector;
		std::vector<Light> m_lights;

		std::unordered_map<entt::entity, RefPtr<BFTexture>> m_shadowMaps;
		std::unordered_set<entt::entity> m_aliveMaps;

		uint32_t m_version = 0;
	};
}