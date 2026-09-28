#pragma once
#include "Core/Common.hpp"
#include "Core/Application.hpp"
#include "Scene/Registry/LightComponent.hpp"
#include "Scene/Registry/TransformComponent.hpp"
#include "Renderer/D3D12Buffer.hpp"

namespace Butterfly
{
	struct GPULight
	{
		uint32_t Type;
		glm::vec3 Color;
		float Range;
		float ConeAngle;
		glm::vec3 Direction;
		glm::vec3 Position;
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
				gpuLight.Type = static_cast<uint32_t>(light.GetType());
				gpuLight.Color = light.GetColor();
				gpuLight.Range = light.GetRange();
				gpuLight.ConeAngle = light.GetConeAngle();
				gpuLight.Direction = transform.GetRotation() * glm::vec3(0.0f, 0.0f, -1.0f);
				gpuLight.Position = transform.GetPosition();
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

		const BFShaderResourceView& SRV() const
		{
			return m_gpuLights->SRV();
		}

		RefPtr<BFStructuredBuffer> m_gpuLights;
		std::vector<GPULight> m_lights;
	};
}