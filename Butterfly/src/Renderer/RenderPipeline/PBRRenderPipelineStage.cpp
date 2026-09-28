#include "Renderer/RenderPipeline/PBRRenderPipelineStage.hpp"
#include "Renderer/Renderer.hpp"

#include "Scene/Scene.hpp"
#include "Scene/Registry/MeshRendererComponent.hpp"
#include "Scene/Registry/TransformComponent.hpp"
#include "Asset/AssetManager.hpp"
#include "Renderer/Camera.hpp"

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
		desc.Flags = BFTextureDesc::ShaderResource;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		std::vector<uint8_t> data = { 255, 255, 255, 255 };
		desc.Data = data.data();
		m_whiteTexture = BFTexture::CreateTextureFromCPUBuffer(desc);
	}

	void PBRRenderPipelineStage::OnRecordPass(const ViewportRenderEvent& ev)
	{
		GraphBuilder& builder = ev.Builder;
		Viewport& viewport = ev.Viewport;

		ForwardRenderer* params = builder.AllocParameters<ForwardRenderer>();

		params->Comp = viewport.RenderTarget.get();

		ev.Viewport.Lights->Update();

		uint32_t entityIndex = 0;
		auto view = Application::Get().GetScene().GetEntityRegistry().view<TransformComponent, MeshRendererComponent>();
		for (auto [entity, transform, meshRenderer] : view.each())
		{
			if (!meshRenderer.GetMeshHandle())
			{
				continue;
			}

			const glm::mat4 model = transform.GetWorldMatrix();
			viewport.ModelMatrices->Write(&model, sizeof(glm::mat4), entityIndex * sizeof(glm::mat4));
			entityIndex++;
		}

		builder.AddPass<ForwardRenderer>("Forward Model",
			[&](const ForwardRenderer& params, D3D12CommandList& list)
			{
				BF_PROFILE_EVENT_DYNAMIC("Forward Model pass");

				BFTexture& rt = *params.Comp;

				// Default Init stuff.
				list.List()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				GraphicsCommands::SetRenderTargets(list, { &rt }, viewport.DepthStencil.get());

				GraphicsCommands::ClearDepthStencil(list, *viewport.DepthStencil);
				GraphicsCommands::ClearRenderTarget(list, rt, { 0.05f, 0.1f, 0.15f, 1.0f });

				GraphicsCommands::SetFullscreenViewportAndRect(list, rt.Width(), rt.Height());

				BFPipelineBuilder psoBuilder;
				psoBuilder.PrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
				psoBuilder.RenderTargetFormats({ DXGI_FORMAT_R8G8B8A8_UNORM });
				psoBuilder.DepthStencilFormat({ DXGI_FORMAT_D24_UNORM_S8_UINT });
				psoBuilder.VertexShader(BFShaderCache::GetOrCreate(L"assets/Shaders/Forward_vert.hlsl", ShaderType::Vertex));
				psoBuilder.PixelShader(BFShaderCache::GetOrCreate(L"assets/Shaders/Forward_frag.hlsl", ShaderType::Pixel));
				psoBuilder.CullingMode(D3D12_CULL_MODE_BACK);

				list.List()->SetPipelineState(psoBuilder.Create().GetHW());

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

					list.List()->IASetIndexBuffer(&mesh->GPUIndices->IBV());

					for (auto& subMesh : mesh->SubMeshes)
					{
						BFTexture* albedo = m_whiteTexture.get();
						if (subMesh.Material.Valid())
						{
							MaterialAsset* material = as.Resolve<MaterialAsset>(subMesh.Material);
							albedo = as.Resolve<TextureAsset>(material->ColorTexture)->Texture.get();
						}

						ShaderVariables()
							.Add(mesh->GPUPositions->SRV().View())
							.Add(mesh->GPUNormals->SRV().View())
							.Add(mesh->GPUUVs->SRV().View())
							.Add(viewport.Uniforms->GetView(HASH("CameraData"))->View())
							.Add(m_defaultSampler->View())
							.Add(albedo->SRV().View())
							.Add(viewport.ModelMatrices->SRV().View())
							.Add(entityIndex)
							.Add(viewport.Lights->SRV().View())
							.Add(viewport.Lights->GetNumLights())
							.Submit(list);

						list.List()->DrawIndexedInstanced(subMesh.IndexCount, 1, subMesh.IndexOffset, 0, 0);
					}

					entityIndex++;
				}
			});
	}
}