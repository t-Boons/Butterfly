#include "Renderer/Light.hpp"
#include "Core/Application.hpp"
#include "Scene/Registry/TransformComponent.hpp"
#include "Renderer/RenderIncludes.hpp"
#include "Renderer/Renderer.hpp"

namespace Butterfly
{
	namespace Utils
	{
		uint32_t LightTypeToGPULightType(LightType type)
		{
			switch (type)
			{
			case LightType::Directional: return 0;
			case LightType::Point: return 1;
			case LightType::Spot: return 2;
			default: return 0;
			}
		}

		LightType GPULightTypeToLightType(uint32_t type)
		{
			switch (type)
			{
			case 0: return LightType::Directional;
			case 1: return LightType::Point;
			case 2: return LightType::Spot;
			default: return LightType::Directional;
			}
		}
	}

	LightBuffer::LightBuffer()
	{
		BFStructuredBufferDesc desc;
		desc.NumElements = static_cast<uint32_t>(MAX_LIGHTS);
		desc.Stride = sizeof(GPULight);
		desc.DebugName = "LightBuffer";
		desc.HeapType = BFHeapType::Upload;
		m_gpuLights = MakeRef<BFStructuredBuffer>(desc);
	}

	void LightBuffer::Update(const Viewport& viewport)
	{
		BF_PROFILE_EVENT()

		uint32_t entityIndex = 0;
		m_numLights = 0;
		for (const auto& [entity, lightComponent, transform] : Application::Get().GetScene().GetEntityRegistry().view<LightComponent, TransformComponent>().each())
		{
			GPULight& light = m_lights[entityIndex];
			light.Type = Utils::LightTypeToGPULightType(lightComponent.GetType());
			light.Color = lightComponent.GetColor();
			light.Range = lightComponent.GetRange();
			light.InnerConeAngleCos = glm::cos(glm::radians(lightComponent.GetInnerConeAngle() * 0.5f));
			light.OuterConeAngleCos = glm::cos(glm::radians(lightComponent.GetOuterConeAngle() * 0.5f));
			light.Position = transform.GetPosition();
			light.Direction = transform.GetForward();

			++entityIndex;
			++m_numLights;

			// Stop evaluating if we have reached the maximum number of lights.
			if (entityIndex >= MAX_LIGHTS)
			{
				break;
			}
		}

		m_gpuLights->Write(m_lights.data(), static_cast<uint32_t>(m_lights.size() * sizeof(GPULight)));
	}

	const std::array<GPULight, 16>& LightBuffer::GetLights() const
	{
		return m_lights;
	}

	const BFShaderResourceView& LightBuffer::SRV() const
	{
		return m_gpuLights->SRV();
	}
}