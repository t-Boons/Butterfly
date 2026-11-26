#include "Renderer/RenderPipeline/PBRRenderPipelineStage.hpp"
#include "Renderer/Renderer.hpp"
#include "Renderer/RenderData.hpp"

#include "Scene/Scene.hpp"
#include "Scene/Registry/MeshRendererComponent.hpp"
#include "Scene/Registry/TransformComponent.hpp"
#include "Asset/AssetManager.hpp"
#include "Renderer/Camera.hpp"
#include "Renderer/Material.hpp"
#include "Renderer/Light.hpp"
#include "Renderer/SharedRenderResources.hpp"

#include "Renderer/RenderPipeline/SkyboxRenderPipelineStage.hpp"
#include "Renderer/RenderPipeline/ShadowRenderPipelineStage.hpp"

namespace Butterfly
{
	PBRRenderPipelineStage::PBRRenderPipelineStage()
	{

	}

	void PBRRenderPipelineStage::OnRecordPass(const ViewportRenderEvent& ev)
	{
		GraphBuilder& builder = ev.Builder;
		Viewport& viewport = ev.Viewport;
		struct ForwardRenderer {};
		ForwardRenderer* params = builder.AllocParameters<ForwardRenderer>();

		int skyboxTextureViewIndex = -1;
		int irradianceTextureViewIndex = -1;
		int prefilterTextureViewIndex = -1;
		if (SkyboxPassParams* skyboxParams = builder.GetPassData<ForwardRenderer, SkyboxPassParams>())
		{
			if (skyboxParams->SkyboxTexture)
			{
				skyboxTextureViewIndex = skyboxParams->SkyboxTexture->SRV(SRVTextureViewInfo::Cubemap()).GetViewIndex();
				irradianceTextureViewIndex = skyboxParams->IrradianceTexture->SRV(SRVTextureViewInfo::Cubemap()).GetViewIndex();
				prefilterTextureViewIndex = skyboxParams->PrefilterTexture->SRV(SRVTextureViewInfo::Cubemap()).GetViewIndex();
			}
		}

		builder.AddPass<ForwardRenderer>("Forward Model",
			[this, viewport, skyboxTextureViewIndex, irradianceTextureViewIndex, prefilterTextureViewIndex](const ForwardRenderer& params, D3D12CommandList& list)
			{
				BF_PROFILE_EVENT_DYNAMIC("Forward Model pass");

				RasterPassStartInfo info;
				info.RenderTarget = &viewport.GetRenderTarget();
				info.DepthStencil = &viewport.GetDepthStencil();
				info.RenderTargetLoadOp = LoadOP::Load;
				info.DepthStencilLoadOp = LoadOP::Load;

				list.StartRenderPass(info, "Forward Model Pass");

				GraphicsCommands::SetFullscreenViewportAndRect(list, info.RenderTarget->Width(), info.RenderTarget->Height());

				GraphicsPSOInfo psoInfo;
				psoInfo.DepthStencil.EnableDepth = true;
				psoInfo.Rasterizer.CullMode = D3D12_CULL_MODE_BACK;
				psoInfo.VertexShader = ShaderCache::GetOrCreate("Forward_vert.hlsl");
				psoInfo.PixelShader = ShaderCache::GetOrCreate("Forward_frag.hlsl", {{"USE_PCSS_SHADOWS"}});

				list.SetGraphicsPSO(psoInfo);

				SamplerDesc pcssSamplerDesc;
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

				Sampler& pcssShadowMapSampler = SamplerCache::GetOrCreate(pcssSamplerDesc);

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
						mesh->GetSDF()->GetSDFTexture()->Resource()->TransitionAll(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
					}
				}

				uint32_t entityIndex = 0;
				for (const RenderObject& renderObject : viewport.GetRenderData().CulledRenderObjects)
				{
					AssetManager& as = Application::Get().GetAssetManager();
					MeshAsset* mesh = as.Resolve<MeshAsset>(renderObject.MeshAsset);

					list.SetIndexBuffer(*mesh->GPUIndices);

					ShaderVariableBuilder(list)
					.Add(viewport.GetRenderData().FrameDataBuffer->CBV())

					.Add(renderObject.ModelDataArrayIndex)
					.Add(renderObject.ModelDataArrayIndex)
					.Add(viewport.GetRenderData().Materials.GetMaterialIndex(renderObject.SubMesh.Material.GetID()))

					.Add(skyboxTextureViewIndex)
					.Add(irradianceTextureViewIndex)
					.Add(prefilterTextureViewIndex)
					.Add(Application::Get().GetRenderer().GetSharedRenderResources().GetBRDFLUT()->SRV(SRVTextureViewInfo::Texture2D()))

					.Add(SamplerCache::GetOrCreate(SamplerDesc::LinearClamp()))
					.Add(SamplerCache::GetOrCreate(SamplerDesc::LinearBorder()))
					.Add(pcssShadowMapSampler)
					.Submit();

					list.DrawIndexedInstanced(renderObject.SubMesh.IndexCount, 1, renderObject.SubMesh.IndexOffset, 0, 0);

					entityIndex++;
				}

				list.EndRenderPass();
			});
	}
}