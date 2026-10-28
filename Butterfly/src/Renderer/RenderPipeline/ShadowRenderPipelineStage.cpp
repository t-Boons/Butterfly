#pragma once
#include "Renderer/RenderPipeline/ShadowRenderPipelineStage.hpp"
#include "Renderer/RenderIncludes.hpp"
#include "Renderer/Renderer.hpp"

namespace Butterfly
{
	void ShadowRenderPipelineStage::OnRecordPass(const ViewportRenderEvent& ev)
	{
		GraphBuilder& builder = ev.Builder;
		Viewport& viewport = ev.Viewport;

		ShadowMapPass* params = builder.AllocParameters<ShadowMapPass>();

		builder.AddPass<ShadowMapPass>("ShadowMap",
			[viewport, this](const ShadowMapPass& params, D3D12CommandList& list)
			{
				const std::vector<Light>& lights = viewport.Lights->GetLights();
				const auto& firstShadowLight = std::find_if(lights.begin(), lights.end(), [](const Light& l) { return l.ShadowMap.ShadowMapTexture != nullptr; });
				if (firstShadowLight == lights.end())
				{
					return;
				}


				RasterPassStartInfo info;
				info.DepthStencil = firstShadowLight->ShadowMap.ShadowMapTexture;
				info.DepthStencilLoadOp = LoadOP::Clear;
				info.DepthValue = 1.0f;
				list.StartRenderPass(info, "ShadowMap Pass");

				list.SetViewport(RenderViewport::FromTexture(*firstShadowLight->ShadowMap.ShadowMapTexture));

				BFGraphicsPSOInfo pso;
				pso.Rasterizer.CullMode = D3D12_CULL_MODE_BACK;
				pso.DepthStencil.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
				pso.DepthStencil.WriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
				pso.DepthStencil.EnableDepth = true;
				pso.VertexShader = BFShaderCache::GetOrCreate("assets/Shaders/ShadowMap_vert.hlsl");
				list.SetGraphicsPSO(pso);

				for (uint32_t lightIndex = 0; lightIndex < lights.size(); lightIndex++)
				{
					const ShadowMap& shadowMap = lights[lightIndex].ShadowMap;

					if (!shadowMap.ShadowMapTexture)
					{
						continue;
					}

					for (uint32_t cascadeIndex = 0; cascadeIndex < shadowMap.NumCascades; cascadeIndex++)
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
									.Add(viewport.Lights->SRV().View())
									.Add(lightIndex)
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

				// Transition the resources so they can be used for shadowmapping.
				for (uint32_t lightIndex = 0; lightIndex < lights.size(); lightIndex++)
				{
					const ShadowMap& shadowMap = lights[lightIndex].ShadowMap;
					if (!shadowMap.ShadowMapTexture)
					{
						continue;
					}
					for (uint32_t cascadeIndex = 0; cascadeIndex < shadowMap.NumCascades; cascadeIndex++)
					{
						shadowMap.ShadowMapTexture->Resource()->Transition(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
					}
				}

				list.EndRenderPass();
			});
	}
}