#include "Renderer/ShadowMap.hpp"
#include "Core/Application.hpp"
#include "Scene/Registry/TransformComponent.hpp"
#include "Renderer/RenderIncludes.hpp"
#include "Renderer/Renderer.hpp"
#include "Renderer/RenderData.hpp"

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

			if (type == LightType::Directional)
			{
				desc.Type = BFTextureType::Texture2DArray;
				desc.ArraySize = MAX_CASCADES;
			}

			if (type == LightType::Point)
			{
				desc.Type = BFTextureType::Cubemap;
				desc.ArraySize = CUBEMAP_SLICES;
			}

			if (type == LightType::Spot)
			{
				desc.Type = BFTextureType::Texture2D;
				desc.ArraySize = 1;
			}

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

		void GetCascadeSplitDirectionalLight(const std::array<float, MAX_CASCADES>& cascadeSplits, const std::array<glm::vec3, 8>& frustumCorners, float cameraNear, float cameraFar, const glm::vec3& lightDirection, float shadowMapSize, std::array<glm::mat4, CUBEMAP_SLICES>& lightProjections, std::array<float, MAX_CASCADES>& texelSizes)
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


		void ToGPUShadowMap(const ShadowMap& shadowMap, GPUShadowMap& gpuShadowMap)
		{
			gpuShadowMap.NumSlices = shadowMap.NumSlices;
			gpuShadowMap.Resolution = shadowMap.Resolution;
			gpuShadowMap.ShadowMapTextureIndex = shadowMap.ShadowMapTexture ? shadowMap.ShadowMapTexture->SRV().View() : -1;
			gpuShadowMap.CascadeSplits = shadowMap.CascadeSplits;
			gpuShadowMap.TexelSizes = shadowMap.TexelSizes;
			gpuShadowMap.LightViewProjections = shadowMap.LightViewProjections;
		}
	}

	ShadowMapBuffer::ShadowMapBuffer()
	{
		BFStructuredBufferDesc desc;
		desc.NumElements = static_cast<uint32_t>(MAX_SHADOW_MAPS);
		desc.Stride = sizeof(GPUShadowMap);
		desc.DebugName = "ShadowMapBuffer";
		desc.HeapType = BFHeapType::Upload;
		m_gpuShadowMapsBuffer = MakeRef<BFStructuredBuffer>(desc);
	}

	void ShadowMapBuffer::Update(const Viewport& viewport)
	{
		BF_PROFILE_EVENT()

		uint32_t entityIndex = 0;
		for (const auto& [entity, lightComponent, transform] : Application::Get().GetScene().GetEntityRegistry().view<LightComponent, TransformComponent>().each())
		{
			ShadowMap& shadow = m_shadowMaps[entityIndex];
			entityIndex++;

			shadow.Resolution = static_cast<float>(lightComponent.GetShadowMapResolution());

			if (lightComponent.GetCastShadows())
			{
				if (lightComponent.GetType() == LightType::Directional)
				{
					shadow.NumSlices = lightComponent.GetNumCascades();
					shadow.CascadeSplits = lightComponent.GetCascadeSplits();

					const auto& cam = viewport.GetRenderData().CameraData;
					const auto corners = Utils::GetFrustumCorners(cam.ViewProjection);
					Utils::GetCascadeSplitDirectionalLight(lightComponent.GetCascadeSplits(), corners, cam.Near, cam.Far, transform.GetForward(), lightComponent.GetShadowMapResolution(),
						shadow.LightViewProjections, shadow.TexelSizes);

				}
				else if (lightComponent.GetType() == LightType::Point)
				{
					shadow.NumSlices = CUBEMAP_SLICES;

					const glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, lightComponent.GetRange());

					const glm::vec3& p = transform.GetPosition();
					shadow.LightViewProjections[0] = shadowProj * glm::lookAt(p, p + glm::vec3(1, 0, 0), glm::vec3(0, 1, 0));
					shadow.LightViewProjections[1] = shadowProj * glm::lookAt(p, p + glm::vec3(-1, 0, 0), glm::vec3(0, 1, 0));
					shadow.LightViewProjections[2] = shadowProj * glm::lookAt(p, p + glm::vec3(0, 1, 0), glm::vec3(0, 0, -1));
					shadow.LightViewProjections[3] = shadowProj * glm::lookAt(p, p + glm::vec3(0, -1, 0), glm::vec3(0, 0, 1));
					shadow.LightViewProjections[4] = shadowProj * glm::lookAt(p, p + glm::vec3(0, 0, 1), glm::vec3(0, 1, 0));
					shadow.LightViewProjections[5] = shadowProj * glm::lookAt(p, p + glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));
				}
				else if (lightComponent.GetType() == LightType::Spot)
				{
					shadow.NumSlices = 1;

					float deg = glm::min(lightComponent.GetOuterConeAngle(), 175.0f);
					const glm::mat4 shadowProj = glm::perspective(glm::radians(deg), 1.0f, 0.1f, lightComponent.GetRange());
					shadow.LightViewProjections[0] = shadowProj * glm::lookAt(transform.GetPosition(), transform.GetPosition() + transform.GetForward(), glm::vec3(0.0f, 1.0f, 0.0f));
				}


				if (!shadow.ShadowMapTexture || shadow.NumSlices != shadow.ShadowMapTexture->Desc().ArraySize || shadow.Resolution != shadow.ShadowMapTexture->Desc().Width)
				{
					shadow.ShadowMapTexture = MakeRef<BFTexture>(Utils::ShadowMapTextureDesc(static_cast<int>(shadow.Resolution), "ShadowMap", lightComponent.GetType()));
				}
			}
			else
			{
				shadow.ShadowMapTexture = nullptr;
				shadow.NumSlices = -1;
				shadow.Resolution = 0.0f;
			}
		}

		// Clear out any remaining shadow maps that are not used.
		for (uint32_t i = entityIndex; i < MAX_SHADOW_MAPS; i++)
		{
			m_shadowMaps[i].NumSlices = -1;
			m_shadowMaps[i].Resolution = 0.0f;
			m_shadowMaps[i].ShadowMapTexture = nullptr;
		}

		for (uint32_t i = 0; i < entityIndex; i++)
		{
			Utils::ToGPUShadowMap(m_shadowMaps[i], m_gpuShadowMaps[i]);
		}

		m_gpuShadowMapsBuffer->Write(m_gpuShadowMaps.data(), static_cast<uint32_t>(m_gpuShadowMaps.size() * sizeof(GPUShadowMap)));
	}

	const BFShaderResourceView& ShadowMapBuffer::SRV() const
	{
		return m_gpuShadowMapsBuffer->SRV();
	}

}