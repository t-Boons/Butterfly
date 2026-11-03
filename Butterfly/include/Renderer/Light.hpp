#pragma once
#include "Core/Common.hpp"
#include "Scene/Registry/LightComponent.hpp"

#define MAX_LIGHTS 16

namespace Butterfly
{
	class BFStructuredBuffer;
	class BFTexture;

	struct GPULight
	{
		uint32_t Type;
		glm::vec3 Color;
		glm::vec3 Position;
		glm::vec3 Direction;
		float Range;
		float InnerConeAngleCos;
		float OuterConeAngleCos;
	};

	class Viewport;
	class BFShaderResourceView;
	class LightBuffer
	{
	public:
		LightBuffer();

		void Update(const Viewport& viewport);

		const std::array<GPULight, MAX_LIGHTS>& GetLights() const;
		const BFShaderResourceView& SRV() const;
		uint32_t GetNumLights() const { return m_numLights; }

	private:
		RefPtr<BFStructuredBuffer> m_gpuLights;
		std::array<GPULight, MAX_LIGHTS> m_lights;
		uint32_t m_numLights = 0;
	};
}