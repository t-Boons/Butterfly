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

		const std::array<glm::vec3, 8> frustumCorners = Utils::GetFrustumCorners(viewport.UniformCameraData.ViewProjection);

		const std::array<float, 4> cascadeSplits
		{
			8,
			16,
			32,
			64,
		};

		std::vector<GPUShadowMap> gpuShadowMaps;

		for(uint32_t i = 0; i < viewport.Lights->GetLights().size(); ++i)
		{
			const GPULight& light = viewport.Lights->GetLights()[i];

			if(Utils::GPULightTypeToLightType(light.Type) == LightType::Directional)
			{
				const std::array<glm::mat4, NUM_CASCADES> cascadeViewProjections = Utils::GetCascadeSplitDirectionalLight(cascadeSplits, frustumCorners, viewport.UniformCameraData.Near, viewport.UniformCameraData.Far, light.Direction, 512);

				m_shadowMaps.resize(1);
				gpuShadowMaps.resize(1);
				for (uint32_t j = 0; j < 1; ++j)
				{
					ShadowMap& map = m_shadowMaps[j];
					GPUShadowMap& gpuMap = gpuShadowMaps[j];

					const std::string name = "ShadowMap LightIndex: " + std::to_string(i);

					BFTextureDesc desc;
					desc.Width = 1024;
					desc.Height = 1024;
					desc.Format = DXGI_FORMAT_D32_FLOAT;
					desc.ViewTypes = BFTextureDesc::ViewType::DepthStencilable | BFTextureDesc::ViewType::ShaderResource;
					desc.DebugName = name;
					desc.Type = BFTextureType::Texture2DArray;
					desc.ArraySize = NUM_CASCADES;

					map.ShadowMapTexture = builder.CreateTransientTexture(name, desc);
					map.NumCascades = NUM_CASCADES;

					gpuMap.ShadowMapTextureIndex = map.ShadowMapTexture->SRV().View();
					gpuMap.NumCascades = NUM_CASCADES;

					map.Size = 1024;
					gpuMap.Size = 1024;

					for (uint32_t k = 0; k < NUM_CASCADES; k++)
					{
						gpuMap.LightViewProjections[k] = cascadeViewProjections[k];
						gpuMap.CascadeSplits[k] = cascadeSplits[k];
						map.LightViewProjections[k] = cascadeViewProjections[k];
						map.CascadeSplits[k] = cascadeSplits[k];
					}
				}
			}
		}

		BFStructuredBufferDesc bufferDesc;
		bufferDesc.DebugName = "ShadowMapBuffer";
		bufferDesc.HeapType = BFHeapType::Upload;
		bufferDesc.Stride = sizeof(GPUShadowMap);
		bufferDesc.NumElements = static_cast<uint32_t>(8);
		BFStructuredBuffer* shadowMapBuffer = builder.CreateTransientStructuredBuffer("ShadowMapBuffer", bufferDesc);
		shadowMapBuffer->Write(gpuShadowMaps.data(), sizeof(GPUShadowMap) * gpuShadowMaps.size());

		ShadowMapPass* params = builder.AllocParameters<ShadowMapPass>();
		params->ShadowMapBuffer = shadowMapBuffer;
		params->NumShadowMaps = static_cast<uint32_t>(gpuShadowMaps.size());

		builder.AddPass<ShadowMapPass>("ShadowMap",
			[viewport, this](const ShadowMapPass& params, D3D12CommandList& list)
			{
				if (m_shadowMaps.empty())
				{
					return;
				}

				RasterPassStartInfo info;
				info.DepthStencil = m_shadowMaps[0].ShadowMapTexture;
				info.DepthStencilLoadOp = LoadOP::Clear;
				info.DepthValue = 1.0f;
				list.StartRenderPass(info, "ShadowMap Pass");

				list.SetViewport(RenderViewport::FromTexture(*m_shadowMaps[0].ShadowMapTexture));

				BFGraphicsPSOInfo pso;
				pso.Rasterizer.CullMode = D3D12_CULL_MODE_BACK;
				pso.DepthStencil.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
				pso.DepthStencil.WriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
				pso.DepthStencil.EnableDepth = true;
				pso.VertexShader = BFShaderCache::GetOrCreate("assets/Shaders/ShadowMap_vert.hlsl", ShaderType::Vertex);
				list.SetGraphicsPSO(pso);

				for (uint32_t shadowMapIndex = 0; shadowMapIndex < params.NumShadowMaps; shadowMapIndex++)
				{
					const ShadowMap& shadowMap = m_shadowMaps[shadowMapIndex];

					for (uint32_t cascadeIndex = 0; cascadeIndex < NUM_CASCADES; cascadeIndex++)
					{
						list.SetDepthStencilTarget(*shadowMap.ShadowMapTexture, cascadeIndex);
						list.ClearDepthStencilTarget(*shadowMap.ShadowMapTexture, 1.0f, 0, cascadeIndex);

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
									.Add(params.ShadowMapBuffer->SRV().View())
									.Add(shadowMapIndex)
									.Add(viewport.Models->SRV().View())
									.Add(viewport.Models->GetModelViewIndex(entityIndex))
									.Add(cascadeIndex)
									.Submit(list);

								list.DrawIndexedInstanced(subMesh.IndexCount, 1, subMesh.IndexOffset, 0, 0);
							}

							entityIndex++;
						}
					}
				}

				m_shadowMaps[0].ShadowMapTexture->Resource()->Transition(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				list.EndRenderPass();
			});
	}
}