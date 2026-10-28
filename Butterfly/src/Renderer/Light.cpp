#include "Renderer/Light.hpp"
#include "Core/Application.hpp"
#include "Scene/Registry/TransformComponent.hpp"
#include "Renderer/RenderIncludes.hpp"
#include "Renderer/Renderer.hpp"

namespace Butterfly
{
	namespace Utils
	{
		BFTextureDesc ShadowMapTextureDesc(int size, const std::string& name, LightType type)
		{
			BFTextureDesc desc;
			desc.Width = size;
			desc.Height = size;
			desc.Format = DXGI_FORMAT_D32_FLOAT;
			desc.ViewTypes = BFTextureDesc::ViewType::DepthStencilable | BFTextureDesc::ViewType::ShaderResource;
			desc.DebugName = name;
			desc.Type = BFTextureType::Texture2DArray;
			desc.ArraySize = MAX_CASCADES;
			return desc;
		}

		std::array<glm::vec3, 8> GetFrustumCorners(const glm::mat4& viewProjection)
		{
			const glm::mat4 inverseViewProjection = glm::inverse(viewProjection);
			std::array<glm::vec3, 8> out;

			int index = 0;
			for (float z : { 0.0f, 1.0f }) for (float y : { -1.0f, 1.0f }) for (float x : { -1.0f, 1.0f })
			{
				glm::vec4 corner = inverseViewProjection * glm::vec4(x, y, z, 1.0f);
				out[index++] = glm::vec3(corner) / corner.w;
			}
			return out;
		}

		void GetCascadeSplitDirectionalLight(const std::array<float, MAX_CASCADES>& cascadeSplits, const std::array<glm::vec3, 8>& frustumCorners, float cameraNear, float cameraFar, const glm::vec3& lightDirection, float shadowMapSize, std::array<glm::mat4, MAX_CASCADES>& lightProjections, std::array<float, MAX_CASCADES>& texelSizes)
		{
			float previousSplit = cameraNear;
			const glm::vec3 direction = glm::normalize(lightDirection);

			glm::vec3 up(0.0f, 1.0f, 0.0f);
			if (glm::abs(glm::dot(direction, up)) > 0.99f)
			{
				up = glm::vec3(0.0f, 0.0f, 1.0f);
			}

			for (uint32_t cascade = 0; cascade < MAX_CASCADES; ++cascade)
			{
				const float splitNear = previousSplit;
				const float splitFar = cascadeSplits[cascade];
				previousSplit = splitFar;

				const float nearT = (splitNear - cameraNear) / (cameraFar - cameraNear);
				const float farT = (splitFar - cameraNear) / (cameraFar - cameraNear);

				std::array<glm::vec3, 8> cascadeCorners;
				for (uint32_t i = 0; i < 4; ++i)
				{
					cascadeCorners[i] = glm::mix(frustumCorners[i], frustumCorners[i + 4], nearT);
					cascadeCorners[i + 4] = glm::mix(frustumCorners[i], frustumCorners[i + 4], farT);
				}

				glm::vec3 center(0.0f);
				for (const glm::vec3& corner : cascadeCorners) center += corner;
				center /= 8.0f;

				float radius = 0.0f;
				for (const glm::vec3& corner : cascadeCorners)
				{
					radius = glm::max(radius, glm::length(corner - center));
				}
				radius = glm::ceil(radius * 16.0f) / 16.0f;

				const float texelSize = (2.0f * radius) / shadowMapSize;
				const glm::mat4 rotView = glm::lookAt(glm::vec3(0.0f), direction, up);

				glm::vec3 centerLS = glm::vec3(rotView * glm::vec4(center, 1.0f));
				centerLS.x = glm::floor(centerLS.x / texelSize) * texelSize;
				centerLS.y = glm::floor(centerLS.y / texelSize) * texelSize;

				center = glm::vec3(glm::inverse(rotView) * glm::vec4(centerLS, 1.0f));

				float farLength = radius * 3.0f;
				const glm::vec3 lightPosition = center - direction * farLength;
				const glm::mat4 lightView = glm::lookAt(lightPosition, center, up);

				const glm::mat4 lightProjection = glm::ortho(-radius, radius, -radius, radius, 0.1f, farLength * 2.0f);
				lightProjections[cascade] = lightProjection * lightView;
				texelSizes[cascade] = texelSize;
			}
		}

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

	GPULight Light::ToGPULight() const
	{
		GPULight gpuLight;
		gpuLight.Type = Utils::LightTypeToGPULightType(Type);
		gpuLight.Color = Color;
		gpuLight.Position = Position;
		gpuLight.Direction = Direction;
		gpuLight.Range = Range;
		gpuLight.InnerConeAngleCos = InnerConeAngleCos;
		gpuLight.OuterConeAngleCos = OuterConeAngleCos;
		gpuLight.ShadowMap.ShadowMapTextureIndex = -1;
		if (ShadowMap.ShadowMapTexture)
		{
			gpuLight.ShadowMap.ShadowMapTextureIndex = ShadowMap.ShadowMapTexture->SRV().View();
			gpuLight.ShadowMap.NumCascades = ShadowMap.NumCascades;
			gpuLight.ShadowMap.Size = ShadowMap.Size;
			gpuLight.ShadowMap.CascadeSplits = ShadowMap.CascadeSplits;
			gpuLight.ShadowMap.LightViewProjections = ShadowMap.LightViewProjections;
			gpuLight.ShadowMap.CascadeTexelSize = ShadowMap.CascadeTexelSize;
		}
		return gpuLight;
	}

	LightBuffer::LightBuffer()
	{
		BFStructuredBufferDesc desc;
		desc.NumElements = static_cast<uint32_t>(16);
		desc.Stride = sizeof(GPULight);
		desc.DebugName = "LightBuffer";
		desc.HeapType = BFHeapType::Upload;
		m_gpuLights = MakeRef<BFStructuredBuffer>(desc);
	}

	void LightBuffer::Update(const Viewport& viewport)
	{
		BF_PROFILE_EVENT()

		m_lights.clear();
		m_gpuLightsVector.clear();
		m_aliveMaps.clear();
		for (const auto& [entity, lightComponent, transform] : Application::Get().GetScene().GetEntityRegistry().view<LightComponent, TransformComponent>().each())
		{
			Light light;
			light.Type = lightComponent.GetType();
			light.Color = lightComponent.GetColor();
			light.Range = lightComponent.GetRange();
			light.InnerConeAngleCos = glm::cos(glm::radians(lightComponent.GetInnerConeAngle()));
			light.OuterConeAngleCos = glm::cos(glm::radians(lightComponent.GetOuterConeAngle()));
			light.Position = transform.GetPosition();
			light.Direction = transform.GetForward();

			if (lightComponent.CastsShadows())
			{
				if (light.Type != LightType::Directional)
				{
					continue;
				}

				light.ShadowMap.NumCascades = lightComponent.GetNumCascades();
				light.ShadowMap.Size = static_cast<float>(lightComponent.GetShadowMapSize());
				light.ShadowMap.CascadeSplits = lightComponent.GetCascadeSplits();

				const auto corners = Utils::GetFrustumCorners(viewport.UniformCameraData.ViewProjection);
				Utils::GetCascadeSplitDirectionalLight(lightComponent.GetCascadeSplits(), corners, viewport.UniformCameraData.Near, viewport.UniformCameraData.Far, light.Direction, lightComponent.GetShadowMapSize(),
					light.ShadowMap.LightViewProjections, light.ShadowMap.CascadeTexelSize);

				auto it = m_shadowMaps.find(entity);
				if (it == m_shadowMaps.end())
				{
					BFTextureDesc desc = Utils::ShadowMapTextureDesc(lightComponent.GetShadowMapSize(), "ShadowMap_" + std::to_string(static_cast<uint32_t>(entity)), lightComponent.GetType());
					RefPtr<BFTexture> shadowMapTexture = MakeRef<BFTexture>(desc);
					m_shadowMaps[entity] = shadowMapTexture;
					it = m_shadowMaps.find(entity);
				}

				light.ShadowMap.ShadowMapTexture = it->second.get();
				m_aliveMaps.insert(entity);
			}


			m_lights.push_back(light);
			m_gpuLightsVector.push_back(light.ToGPULight());
		}

		if (m_gpuLights->NumElements() < m_gpuLightsVector.size())
		{
			BFStructuredBufferDesc desc;
			desc.NumElements = static_cast<uint32_t>(m_gpuLightsVector.size() * 2);
			desc.Stride = sizeof(GPULight);
			desc.DebugName = "LightBuffer";
			desc.HeapType = BFHeapType::Upload;
			m_gpuLights = MakeRef<BFStructuredBuffer>(desc);
		}

		m_gpuLights->Write(m_gpuLightsVector.data(), static_cast<uint32_t>(m_gpuLightsVector.size() * sizeof(GPULight)));
		std::erase_if(m_shadowMaps, [&](const auto& pair) { return !m_aliveMaps.contains(pair.first); });
	}

	const std::vector<Light>& LightBuffer::GetLights() const
	{
		return m_lights;
	}

	const BFShaderResourceView& LightBuffer::SRV() const
	{
		return m_gpuLights->SRV();
	}
}