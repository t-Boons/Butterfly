#pragma once
#include "Core/Common.hpp"
#include "Core/Application.hpp"
#include "Scene/Registry/LightComponent.hpp"
#include "Scene/Registry/TransformComponent.hpp"
#include "Renderer/D3D12Buffer.hpp"

namespace Butterfly
{
	namespace Utils
	{
		inline uint32_t LightTypeToGPULightType(LightType type)
		{
			switch (type)
			{
			case LightType::Directional: return 0;
			case LightType::Point: return 1;
			case LightType::Spot: return 2;
			default: return 0;
			}
		}

		inline LightType GPULightTypeToLightType(uint32_t type)
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

	class LightBuffer
	{
	public:
		LightBuffer()
		{
			BFStructuredBufferDesc desc;
			desc.NumElements = static_cast<uint32_t>(16);
			desc.Stride = sizeof(GPULight);
			desc.DebugName = "LightBuffer";
			desc.HeapType = BFHeapType::Upload;
			m_gpuLights = MakeRef<BFStructuredBuffer>(desc);
		}

		void Update()
		{
			m_lights.clear();
			for (const auto& [entity, light, transform] : Application::Get().GetScene().GetEntityRegistry().view<LightComponent, TransformComponent>().each())
			{
				GPULight gpuLight;
				gpuLight.Type = Utils::LightTypeToGPULightType(light.GetType());
				gpuLight.Color = light.GetColor();
				gpuLight.Range = light.GetRange();
				gpuLight.InnerConeAngleCos = glm::cos(glm::radians(light.GetInnerConeAngle()));
				gpuLight.OuterConeAngleCos = glm::cos(glm::radians(light.GetOuterConeAngle()));
				gpuLight.Position = transform.GetPosition();
				gpuLight.Direction = transform.GetForward();
				m_lights.push_back(gpuLight);
			}

			if (m_gpuLights->NumElements() < m_lights.size())
			{	
				BFStructuredBufferDesc desc;
				desc.NumElements = static_cast<uint32_t>(m_lights.size() * 2);
				desc.Stride = sizeof(GPULight);
				desc.DebugName = "LightBuffer";
				desc.HeapType = BFHeapType::Upload;
				m_gpuLights = MakeRef<BFStructuredBuffer>(desc);
			}

			m_gpuLights->Write(m_lights.data(), static_cast<uint32_t>(m_lights.size() * sizeof(GPULight)));
		}

		uint32_t GetNumLights() const
		{
			return static_cast<uint32_t>(m_lights.size());
		}

		const std::vector<GPULight>& GetLights() const
		{
			return m_lights;
		}

		const BFShaderResourceView& SRV() const
		{
			return m_gpuLights->SRV();
		}

	private:

		RefPtr<BFStructuredBuffer> m_gpuLights;
		std::vector<GPULight> m_lights;
	};
}