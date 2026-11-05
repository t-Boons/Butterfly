#pragma once
#include "Renderer/RenderPipeline/ShadowRenderPipelineStage.hpp"
#include "Renderer/RenderIncludes.hpp"
#include "Renderer/Renderer.hpp"
#include "Renderer/RenderData.hpp"

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
				const std::array<ShadowMap, MAX_SHADOW_MAPS>& shadowMaps = viewport.GetRenderData().ShadowMaps.GetShadowMaps();
				const auto& firstValidShadow = std::find_if(shadowMaps.begin(), shadowMaps.end(), [](const ShadowMap& l) { return l.ShadowMapTexture != nullptr; });
				if (firstValidShadow == shadowMaps.end())
				{
					return;
				}


				RasterPassStartInfo info;
				info.DepthStencil = firstValidShadow->ShadowMapTexture.get();
				info.DepthStencilLoadOp = LoadOP::Clear;
				info.DepthValue = 1.0f;
				list.StartRenderPass(info, "ShadowMap Pass");

				list.SetViewport(RenderViewport::FromTexture(*firstValidShadow->ShadowMapTexture));

				BFGraphicsPSOInfo pso;
				pso.Rasterizer.CullMode = D3D12_CULL_MODE_BACK;
				pso.DepthStencil.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
				pso.DepthStencil.WriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
				pso.DepthStencil.EnableDepth = true;
				pso.VertexShader = BFShaderCache::GetOrCreate("assets/Shaders/ShadowMap_vert.hlsl");
				list.SetGraphicsPSO(pso);

				for (uint32_t shadowMapIndex = 0; shadowMapIndex < MAX_SHADOW_MAPS; shadowMapIndex++)
				{
					const ShadowMap& shadowMap = shadowMaps[shadowMapIndex];

					if (!shadowMap.ShadowMapTexture)
					{
						continue;
					}

					for (uint32_t slice = 0; slice < shadowMap.NumSlices; slice++)
					{
						list.SetDepthStencilTarget(*shadowMap.ShadowMapTexture, slice);
						list.ClearDepthStencilTarget(*shadowMap.ShadowMapTexture, 1.0f, 0, slice);


						for (const auto& renderObject : viewport.GetRenderData().ShadowRenderObjects[shadowMapIndex][slice])
						{
							AssetManager& as = Application::Get().GetAssetManager();
							MeshAsset* mesh = as.Resolve<MeshAsset>(renderObject.MeshAsset);

							list.SetIndexBuffer(*mesh->GPUIndices);

							ShaderVariables()
								.Add(mesh->GPUPositions->SRV().View())
								.Add(viewport.GetRenderData().Uniforms->GetView(HASH("CameraData"))->View())
								.Add(viewport.GetRenderData().ShadowMaps.SRV().View())
								.Add(shadowMapIndex)
								.Add(viewport.GetRenderData().Models.SRV().View())
								.Add(viewport.GetRenderData().Models.GetModelViewIndex(renderObject.MeshAsset))
								.Add(slice)
								.Submit(list);

							list.DrawIndexedInstanced(renderObject.SubMesh.IndexCount, 1, renderObject.SubMesh.IndexOffset, 0, 0);
						}
					}
				}

				// Transition the resources so they can be used for shadowmapping.
				for (uint32_t lightIndex = 0; lightIndex < MAX_SHADOW_MAPS; lightIndex++)
				{
					const ShadowMap& shadowMap = shadowMaps[lightIndex];
					if (!shadowMap.ShadowMapTexture)
					{
						continue;
					}
					for (uint32_t cascadeIndex = 0; cascadeIndex < shadowMap.NumSlices; cascadeIndex++)
					{
						shadowMap.ShadowMapTexture->Resource()->Transition(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
					}
				}

				list.EndRenderPass();
			});
	}
}