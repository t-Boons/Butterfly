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
#include "Renderer/RenderPipeline/ShadowRenderPipelineStage.hpp"

namespace Butterfly
{
	PBRRenderPipelineStage::PBRRenderPipelineStage()
	{
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

		int shadowMapBufferViewIndex = -1;
		int numShadowMaps = 0;
		if (ShadowMapPass* shadowPass = builder.GetPassData<ForwardRenderer, ShadowMapPass>())
		{
			if (shadowPass->ShadowMapBuffer)
			{
				shadowMapBufferViewIndex = shadowPass->ShadowMapBuffer->SRV().View();
				numShadowMaps = shadowPass->NumShadowMaps;
			}
		}
		

		params->Comp = &viewport.GetRenderTarget();

		builder.AddPass<ForwardRenderer>("Forward Model",
			[this, viewport, shadowMapBufferViewIndex, numShadowMaps, skyboxTextureViewIndex](const ForwardRenderer& params, D3D12CommandList& list)
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
				psoInfo.VertexShader = BFShaderCache::GetOrCreate("assets/Shaders/Forward_vert.hlsl", ShaderType::Vertex);
				psoInfo.PixelShader = BFShaderCache::GetOrCreate("assets/Shaders/Forward_frag.hlsl", ShaderType::Pixel, {"USE_PCSS_SHADOWS"});

				list.SetGraphicsPSO(psoInfo);

				BFSamplerDesc pcssSamplerDesc;
				pcssSamplerDesc.Desc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
				pcssSamplerDesc.Desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
				pcssSamplerDesc.Desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
				pcssSamplerDesc.Desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
				pcssSamplerDesc.Desc.MipLODBias = 0.0f;
				pcssSamplerDesc.Desc.MaxAnisotropy = 1;
				pcssSamplerDesc.Desc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
				pcssSamplerDesc.Desc.BorderColor[0] = 1.0f;
				pcssSamplerDesc.Desc.BorderColor[1] = 1.0f;
				pcssSamplerDesc.Desc.BorderColor[2] = 1.0f;
				pcssSamplerDesc.Desc.BorderColor[3] = 1.0f;
				pcssSamplerDesc.Desc.MinLOD = 0.0f;
				pcssSamplerDesc.Desc.MaxLOD = 0.0f;

				BFSampler& pcssShadowMapSampler = BFSamplerCache::GetOrCreate(pcssSamplerDesc);

				// TODO make this cleaner. Right now this is only used because all SDF's start as UNORDERED_ACCEASS
				auto view = Application::Get().GetScene().GetEntityRegistry().view<TransformComponent, MeshRendererComponent>();
				for (const auto& [entity, transform, meshRenderer] : view.each())
				{
					if (!meshRenderer.GetMeshHandle())
					{
						continue;
					}
					AssetManager& as = Application::Get().GetAssetManager();
					MeshAsset* mesh = as.Resolve<MeshAsset>(meshRenderer.GetMeshHandle());
					if (mesh->GetSDF())
					{
						mesh->GetSDF()->GetSDFTexture()->Resource()->Transition(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
					}
				}

				uint32_t entityIndex = 0;
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
							.Add(mesh->GPUNormals->SRV().View())
							.Add(mesh->GPUTangents->SRV().View())
							.Add(mesh->GPUUVs->SRV().View())
							.Add(viewport.Uniforms->GetView(HASH("CameraData"))->View())
							.Add(BFSamplerCache::GetOrCreate(BFSamplerDesc::LinearWrap()).View())
							.Add(viewport.Models->SRV().View())
							.Add(viewport.Models->GetModelViewIndex(entityIndex))
							.Add(viewport.Lights->SRV().View())
							.Add(viewport.Lights->GetNumLights())
							.Add(viewport.Materials->GetMaterialBuffer().SRV().View())
							.Add(viewport.Materials->GetMaterialIndex(subMesh.Material.GetID()))
							.Add(skyboxTextureViewIndex)
							.Add(viewport.Models->GetNumModels())
							.Add(BFSamplerCache::GetOrCreate(BFSamplerDesc::LinearClamp()).View())
							.Add(shadowMapBufferViewIndex)
							.Add(numShadowMaps)
							.Add(BFSamplerCache::GetOrCreate(BFSamplerDesc::LinearBorder()).View())
							.Add(pcssShadowMapSampler.View())
							.Submit(list);

						list.DrawIndexedInstanced(subMesh.IndexCount, 1, subMesh.IndexOffset, 0, 0);
					}

					entityIndex++;
				}

				list.EndRenderPass();
			});
	}
}