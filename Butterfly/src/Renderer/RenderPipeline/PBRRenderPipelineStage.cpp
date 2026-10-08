#include "Renderer/RenderPipeline/PBRRenderPipelineStage.hpp"
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

		int skyboxTextureViewIndex = -1;
		if (SkyboxPassParams* skyboxParams = builder.GetPassData<ForwardRenderer, SkyboxPassParams>())
		{
			if (skyboxParams->SkyboxTexture)
			{
				skyboxTextureViewIndex = skyboxParams->SkyboxTexture->SRV().View();
			}
		}
		

		params->Comp = &viewport.GetRenderTarget();

		builder.AddPass<ForwardRenderer>("Forward Model",
			[&, skyboxTextureViewIndex](const ForwardRenderer& params, D3D12CommandList& list)
			{
				BF_PROFILE_EVENT_DYNAMIC("Forward Model pass");

				RasterPassStartInfo info;
				info.RenderTarget = params.Comp;
				info.DepthStencil = &viewport.GetDepthStencil();
				info.RenderTargetLoadOp = LoadOP::Load;
				info.DepthStencilLoadOp = LoadOP::Load;

				list.StartRenderPass(info, "Forward Model Pass");

				GraphicsCommands::SetFullscreenViewportAndRect(list, info.RenderTarget->Width(), info.RenderTarget->Height());

				BFGraphicsPSOInfo psoInfo;
				psoInfo.DepthStencil.EnableDepth = true;
				psoInfo.Rasterizer.CullMode = D3D12_CULL_MODE_BACK;
				psoInfo.VertexShader = BFShaderCache::GetOrCreate(L"assets/Shaders/Forward_vert.hlsl", ShaderType::Vertex);
				psoInfo.PixelShader = BFShaderCache::GetOrCreate(L"assets/Shaders/Forward_frag.hlsl", ShaderType::Pixel);

				list.SetGraphicsPSO(psoInfo);

				D3D12_SAMPLER_DESC samplerDesc = {};
				samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
				samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
				samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
				samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
				samplerDesc.MipLODBias = 0.0f;
				samplerDesc.MaxAnisotropy = 1; 
				samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NONE;
				samplerDesc.BorderColor[0] = 0.0f;
				samplerDesc.BorderColor[1] = 0.0f;
				samplerDesc.BorderColor[2] = 0.0f;
				samplerDesc.BorderColor[3] = 0.0f;
				samplerDesc.MinLOD = 0.0f;
				samplerDesc.MaxLOD = 0.0f;
				BFSampler sdfSampler(&samplerDesc);


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

					mesh->SDF->Resource()->Transition(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

					for (auto& subMesh : mesh->SubMeshes)
					{
						ShaderVariables()
							.Add(mesh->GPUPositions->SRV().View())
							.Add(mesh->GPUNormals->SRV().View())
							.Add(mesh->GPUTangents->SRV().View())
							.Add(mesh->GPUUVs->SRV().View())
							.Add(viewport.Uniforms->GetView(HASH("CameraData"))->View())
							.Add(m_defaultSampler->View())
							.Add(viewport.Models->SRV().View())
							.Add(viewport.Models->GetModelViewIndex(entityIndex))
							.Add(viewport.Lights->SRV().View())
							.Add(viewport.Lights->GetNumLights())
							.Add(viewport.Materials->GetMaterialBuffer().SRV().View())
							.Add(viewport.Materials->GetMaterialIndex(subMesh.Material.GetID()))
							.Add(skyboxTextureViewIndex)
							.Add(viewport.Models->GetNumModels())
							.Add(sdfSampler.View())
							.Submit(list);

						list.DrawIndexedInstanced(subMesh.IndexCount, 1, subMesh.IndexOffset, 0, 0);
					}

					entityIndex++;
				}

				list.EndRenderPass();
			});
	}
}