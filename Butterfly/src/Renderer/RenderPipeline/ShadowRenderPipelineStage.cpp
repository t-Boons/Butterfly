#pragma once
#include "Renderer/RenderPipeline/ShadowRenderPipelineStage.hpp"
#include "Renderer/RenderIncludes.hpp"
#include "Renderer/Renderer.hpp"

namespace Butterfly
{
	namespace Utils
	{
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

		std::array<glm::mat4, NUM_CASCADES> GetCascadeSplitDirectionalLight(const std::array<float, NUM_CASCADES>& cascadeSplits,
			const std::array<glm::vec3, 8>& frustumCorners,
			float cameraNear, float cameraFar,
			const glm::vec3& lightDirection,
			float shadowMapSize)
		{
			std::array<glm::mat4, NUM_CASCADES> out;
			float previousSplit = cameraNear;
			const glm::vec3 direction = glm::normalize(lightDirection);

			glm::vec3 up(0.0f, 1.0f, 0.0f);
			if (glm::abs(glm::dot(direction, up)) > 0.99f)
			{
				up = glm::vec3(0.0f, 0.0f, 1.0f);
			}

			for (uint32_t cascade = 0; cascade < NUM_CASCADES; ++cascade)
			{
				const float splitNear = previousSplit;
				const float splitFar = cascadeSplits[cascade];
				previousSplit = splitFar;

				const float nearT = (splitNear - cameraNear) / (cameraFar - cameraNear);
				const float farT = (splitFar - cameraNear) / (cameraFar - cameraNear);

				// Slice the camera frustum for this cascade
				std::array<glm::vec3, 8> cascadeCorners;
				for (uint32_t i = 0; i < 4; ++i)
				{
					cascadeCorners[i] = glm::mix(frustumCorners[i], frustumCorners[i + 4], nearT);
					cascadeCorners[i + 4] = glm::mix(frustumCorners[i], frustumCorners[i + 4], farT);
				}

				// Center + bounding sphere radius (constant size, no shimmering on rotation)
				glm::vec3 center(0.0f);
				for (const glm::vec3& corner : cascadeCorners) center += corner;
				center /= 8.0f;

				float radius = 0.0f;
				for (const glm::vec3& corner : cascadeCorners)
				{
					radius = glm::max(radius, glm::length(corner - center));
				}
				radius = glm::ceil(radius * 16.0f) / 16.0f;

				// Pull the light back so casters outside the view still land in the map
				const float casterExtend = radius * 2.0f; // or use your scene bounds
				const glm::vec3 lightPosition = center - direction * (radius + casterExtend);
				const glm::mat4 lightView = glm::lookAt(lightPosition, center, up);

				glm::mat4 lightProjection = glm::ortho(-radius, radius, -radius, radius, 0.01f, 2.0f * radius + casterExtend);

				// Texel snapping: shift the projection so the world origin lands on a texel boundary
				const glm::mat4 shadowMatrix = lightProjection * lightView;
				glm::vec4 origin = shadowMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
				origin *= shadowMapSize * 0.5f;
				const glm::vec2 rounded = glm::round(glm::vec2(origin));
				const glm::vec2 offset = (rounded - glm::vec2(origin)) * (2.0f / shadowMapSize);
				lightProjection[3][0] += offset.x;
				lightProjection[3][1] += offset.y;

				out[cascade] = lightProjection * lightView;
			}

			return out;
		}
	}

	void ShadowRenderPipelineStage::OnRecordPass(const ViewportRenderEvent& ev)
	{
		GraphBuilder& builder = ev.Builder;
		Viewport& viewport = ev.Viewport;

		if (!m_shadowMapTexture)
		{
			BFTextureDesc desc;
			desc.Width = 512;
			desc.Height = 512;
			desc.Format = DXGI_FORMAT_D32_FLOAT;
			desc.ViewTypes = BFTextureDesc::ViewType::DepthStencilable | BFTextureDesc::ViewType::ShaderResource;
			desc.DebugName = "ShadowMap";
			desc.Type = BFTextureType::Texture2DArray;
			desc.ArraySize = NUM_CASCADES;
			m_shadowMapTexture = MakeRef<BFTexture>(desc);
		}

		ShadowMapParameters* params = builder.AllocParameters<ShadowMapParameters>();
		params->Map.ShadowMapTexture = m_shadowMapTexture.get();

		struct ShadowLightSource
		{
			glm::mat4 ViewProj[NUM_CASCADES];
		} shadowLightSource;


		GPULight directionalLight;
		bool foundDirectionalLight = false;
		for(const auto& light : viewport.Lights->GetLights())
		{
			if(Utils::GPULightTypeToLightType(light.Type) == LightType::Directional)
			{
				directionalLight = light;
				foundDirectionalLight = true;
				break;
			}
		}

		if (foundDirectionalLight)
		{
			const std::array<float, 4> cascadeSplits
			{
				8,
				16,
				32,
				64,
			};

			const std::array<glm::vec3, 8> frustumCorners = Utils::GetFrustumCorners(viewport.UniformCameraData.ViewProjection);
			const std::array<glm::mat4, NUM_CASCADES> cascadeViewProjections = Utils::GetCascadeSplitDirectionalLight(
				cascadeSplits, frustumCorners,
				viewport.UniformCameraData.Near, viewport.UniformCameraData.Far, directionalLight.Direction, 512);


			for (uint32_t i = 0; i < NUM_CASCADES; i++)
			{
				shadowLightSource.ViewProj[i] = cascadeViewProjections[i];
				m_shadowMaps.LightViewProjections[i] = cascadeViewProjections[i];
			}
		}



		viewport.Uniforms->GetOrCreateView(sizeof(ShadowLightSource), HASH("ShadowLightSource"));
		viewport.Uniforms->Write(&shadowLightSource, sizeof(ShadowLightSource), HASH("ShadowLightSource"));

		builder.AddPass<ShadowMapParameters>("ShadowMap",
			[viewport, this](const ShadowMapParameters& params, D3D12CommandList& list)
			{
				RasterPassStartInfo info;
				info.DepthStencil = params.Map.ShadowMapTexture;
				info.DepthStencilLoadOp = LoadOP::Clear;
				info.DepthValue = 1.0f;
				list.StartRenderPass(info, "ShadowMap Pass");

				list.SetViewport(RenderViewport::FromTexture(*params.Map.ShadowMapTexture));

				BFGraphicsPSOInfo pso;
				pso.Rasterizer.CullMode = D3D12_CULL_MODE_BACK;
				pso.DepthStencil.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
				pso.DepthStencil.WriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
				pso.DepthStencil.EnableDepth = true;
				pso.VertexShader = BFShaderCache::GetOrCreate(L"assets/Shaders/ShadowMap_vert.hlsl", ShaderType::Vertex);
				list.SetGraphicsPSO(pso);

				for (uint32_t cascadeIndex = 0; cascadeIndex < NUM_CASCADES; cascadeIndex++)
				{
					list.SetDepthStencilTarget(*params.Map.ShadowMapTexture, cascadeIndex);
					list.ClearDepthStencilTarget(*params.Map.ShadowMapTexture, 1.0f, 0, cascadeIndex);

					uint32_t entityIndex = 0;
					auto view = Application::Get().GetScene().GetEntityRegistry().view<TransformComponent, MeshRendererComponent>();
					for (const auto& [entity, transform, meshRenderer] : view.each())
					{
						if (!meshRenderer.GetMeshHandle())
						{
							continue;
						}

						AssetManager& as = Application::Get().GetAssetManager();
						MeshAsset* mesh = as.Resolve<MeshAsset>(meshRenderer.GetMeshHandle());

						list.SetIndexBuffer(*mesh->GPUIndices);

						for (auto& subMesh : mesh->SubMeshes)
						{
							ShaderVariables()
								.Add(mesh->GPUPositions->SRV().View())
								.Add(viewport.Uniforms->GetView(HASH("CameraData"))->View())
								.Add(viewport.Uniforms->GetView(HASH("ShadowLightSource"))->View())
								.Add(viewport.Models->SRV().View())
								.Add(viewport.Models->GetModelViewIndex(entityIndex))
								.Add(cascadeIndex)
								.Submit(list);

							list.DrawIndexedInstanced(subMesh.IndexCount, 1, subMesh.IndexOffset, 0, 0);
						}

						entityIndex++;
					}
				}

				params.Map.ShadowMapTexture->Resource()->Transition(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				list.EndRenderPass();
			});
	}
}