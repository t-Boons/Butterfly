#include "Renderer/RenderPipeline/PBRRenderPipelineStage.hpp"
#include "Renderer/Renderer.hpp"

#include "Scene/Scene.hpp"
#include "Scene/Registry/MeshRendererComponent.hpp"
#include "Scene/Registry/TransformComponent.hpp"
#include "Asset/AssetManager.hpp"
#include "Renderer/Camera.hpp"
#include "Renderer/Material.hpp"
#include "Renderer/Light.hpp"

namespace Butterfly
{
	PBRRenderPipelineStage::PBRRenderPipelineStage()
	{
		m_defaultSampler = MakeRef<BFSampler>();
		BFTextureDesc desc;
		desc.DebugName = "WhiteTexture";
		desc.Width = 1;
		desc.Height = 1;
		desc.ViewTypes = BFTextureDesc::ViewType::ShaderResource;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		std::vector<uint8_t> data = { 255, 255, 255, 255 };
		desc.UploadData.CPUCopySource = data.data();
		m_whiteTexture = MakeRef<BFTexture>(desc);
	}

	void PBRRenderPipelineStage::OnRecordPass(const ViewportRenderEvent& ev)
	{
		GraphBuilder& builder = ev.Builder;
		Viewport& viewport = ev.Viewport;

		ForwardRenderer* params = builder.AllocParameters<ForwardRenderer>();

		params->Comp = &viewport.GetRenderTarget();

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

			const glm::mat4 model = transform.GetWorldMatrix();
			ModelMatrixData modelData;
			modelData.ModelMatrix = model;
			modelData.NormalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
			viewport.ModelMatrices->Write(&modelData, sizeof(ModelMatrixData), entityIndex * sizeof(ModelMatrixData));

			entityIndex++;
		}

		builder.AddPass<ForwardRenderer>("Forward Model",
			[&](const ForwardRenderer& params, D3D12CommandList& list)
			{
				BF_PROFILE_EVENT_DYNAMIC("Forward Model pass");

				RasterPassStartInfo info;
				info.RenderTarget = params.Comp;
				info.DepthStencil = &viewport.GetDepthStencil();
				info.ClearColor = { 0.05f, 0.1f, 0.15f, 1.0f };
				info.DepthValue = 1.0f;
				info.RenderTargetLoadOp = LoadOP::Clear;
				info.DepthStencilLoadOp = LoadOP::Clear;

				list.StartRenderPass(info, "Forward Model Pass");

				GraphicsCommands::SetFullscreenViewportAndRect(list, info.RenderTarget->Width(), info.RenderTarget->Height());

				BFGraphicsPSOInfo psoInfo;
				psoInfo.DepthStencil.EnableDepth = true;
				psoInfo.Rasterizer.CullMode = D3D12_CULL_MODE_BACK;
				psoInfo.VertexShader = BFShaderCache::GetOrCreate(L"assets/Shaders/Forward_vert.hlsl", ShaderType::Vertex);
				psoInfo.PixelShader = BFShaderCache::GetOrCreate(L"assets/Shaders/Forward_frag.hlsl", ShaderType::Pixel);

				list.SetGraphicsPSO(psoInfo);

				uint32_t entityIndex = 0;
				auto view = Application::Get().GetScene().GetEntityRegistry().view<TransformComponent, MeshRendererComponent>();
				for (auto [entity, transform, meshRenderer] : view.each())
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
							.Add(mesh->GPUNormals->SRV().View())
							.Add(mesh->GPUTangents->SRV().View())
							.Add(mesh->GPUUVs->SRV().View())
							.Add(viewport.Uniforms->GetView(HASH("CameraData"))->View())
							.Add(m_defaultSampler->View())
							.Add(viewport.ModelMatrices->SRV().View())
							.Add(entityIndex)
							.Add(viewport.Lights->SRV().View())
							.Add(viewport.Lights->GetNumLights())
							.Add(viewport.Materials->GetMaterialBuffer().SRV().View())
							.Add(viewport.Materials->GetMaterialIndex(subMesh.Material.GetID()))
							.Submit(list);

						list.DrawIndexedInstanced(subMesh.IndexCount, 1, subMesh.IndexOffset, 0, 0);
					}

					entityIndex++;
				}

				list.EndRenderPass();
			});
	}
}