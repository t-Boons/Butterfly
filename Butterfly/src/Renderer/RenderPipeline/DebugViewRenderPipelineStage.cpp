#include "Renderer/RenderPipeline/DebugViewRenderPipelineStage.hpp"
#include "Renderer/Renderer.hpp"

#include "Scene/Scene.hpp"
#include "Scene/Registry/MeshRendererComponent.hpp"
#include "Scene/Registry/TransformComponent.hpp"
#include "Asset/AssetManager.hpp"
#include "Renderer/Camera.hpp"
#include "Renderer/Material.hpp"
#include "Renderer/Light.hpp"

#include "Renderer/RenderPipeline/SkyboxRenderPipelineStage.hpp"

namespace Butterfly
{
	void DebugViewRenderPipelineStage::OnRecordPass(const ViewportRenderEvent& ev)
	{
		GraphBuilder& builder = ev.Builder;
		Viewport& viewport = ev.Viewport;

		struct DebugViewPassData
		{ };

		DebugViewPassData* data = builder.AllocParameters<DebugViewPassData>();

		ev.Viewport.Lights->Update();
		ev.Viewport.Materials->Validate();

		uint32_t entityIndex = 0;
		auto view = Application::Get().GetScene().GetEntityRegistry().view<TransformComponent, MeshRendererComponent>();
		for (auto [entity, transform, meshRenderer] : view.each())
		{
			if (!meshRenderer.GetMeshHandle())
			{
				continue;
			}

			MeshAsset* mesh = Application::Get().GetAssetManager().Resolve<MeshAsset>(meshRenderer.GetMeshHandle());

			const glm::mat4 model = transform.GetWorldMatrix();
			ModelMatrixData modelData;
			modelData.ModelMatrix = model;
			modelData.NormalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
			modelData.BoundsMin = mesh->Bounds.Min;
			modelData.BoundsSize = mesh->Bounds.Size();
			modelData.SDFResolution = mesh->SDFResolution;
			viewport.ModelMatrices->Write(&modelData, sizeof(ModelMatrixData), entityIndex * sizeof(ModelMatrixData));


			entityIndex++;
		}

		builder.AddPass<DebugViewPassData>("DebugViewPass", [&](const DebugViewPassData& data, D3D12CommandList& list)
			{
				BF_PROFILE_EVENT_DYNAMIC("Debug View pass");

				RasterPassStartInfo info;
				info.RenderTarget = &viewport.GetRenderTarget();
				info.DepthStencil = &viewport.GetDepthStencil();
				info.RenderTargetLoadOp = LoadOP::Load;
				info.DepthStencilLoadOp = LoadOP::Load;

				list.StartRenderPass(info, "Debug View Pass");
				list.SetViewport(RenderViewport::FromTexture(viewport.GetRenderTarget()));

				uint32_t entityIndex = 0;
				auto view = Application::Get().GetScene().GetEntityRegistry().view<TransformComponent, MeshRendererComponent>();
				for (auto [entity, transform, meshRenderer] : view.each())
				{
					if (!meshRenderer.GetMeshHandle())
					{
						continue;
					}

					BFGraphicsPSOInfo psoInfo;
					psoInfo.DepthStencil.EnableDepth = true;
					psoInfo.Rasterizer.CullMode = D3D12_CULL_MODE_BACK;
					psoInfo.VertexShader = BFShaderCache::GetOrCreate(L"assets/Shaders/DebugView_vert.hlsl", ShaderType::Vertex);
					psoInfo.PixelShader = BFShaderCache::GetOrCreate(L"assets/Shaders/DebugView_frag.hlsl", ShaderType::Pixel);

					list.SetGraphicsPSO(psoInfo);

					AssetManager& as = Application::Get().GetAssetManager();
					MeshAsset* mesh = as.Resolve<MeshAsset>(meshRenderer.GetMeshHandle());

					list.SetIndexBuffer(*mesh->GPUIndices);


					mesh->SDF->Resource()->Transition(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

					BFSampler smp;
					for (auto& subMesh : mesh->SubMeshes)
					{
						ShaderVariables()
							.Add(mesh->GPUPositions->SRV().View())
							.Add(mesh->GPUNormals->SRV().View())
							.Add(mesh->GPUTangents->SRV().View())
							.Add(mesh->GPUUVs->SRV().View())
							.Add(viewport.Uniforms->GetView(HASH("CameraData"))->View())
							.Add(smp.View())
							.Add(viewport.ModelMatrices->SRV().View())
							.Add(entityIndex)
							.Add(viewport.Lights->SRV().View())
							.Add(viewport.Lights->GetNumLights())
							.Add(viewport.Materials->GetMaterialBuffer().SRV().View())
							.Add(viewport.Materials->GetMaterialIndex(subMesh.Material.GetID()))
							.Add(static_cast<int>(m_debugViewType))
							.Add(mesh->SDF->SRV().View())
							.Submit(list);

						list.DrawIndexedInstanced(subMesh.IndexCount, 1, subMesh.IndexOffset, 0, 0);
					}

					entityIndex++;
				}

				list.EndRenderPass();
			});
	}
}