#pragma once
#include "Renderer/RenderPipeline/SkyboxRenderPipelineStage.hpp"
#include "Renderer/RenderPipeline/RenderPipeline.hpp"
#include "Renderer/Renderer.hpp"
#include "Renderer/D3D12Sampler.hpp"
#include "Renderer/D3D12/D3D12CommandQueue.hpp"

namespace Butterfly
{
	void SkyboxRenderPipelineStage::LoadSkybox(const SkyboxComponent& component)
	{
		uint32_t width, height = 0;
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
		if (component.GetType() == SkyboxType::Cubemap)
		{

			std::array<RefPtr<BFTexture>, 6> textures;
			for (uint32_t i = 0; i < 6; ++i)
			{
				TextureAsset* asset = Application::Get().GetAssetManager().Resolve<TextureAsset>(component.GetTextureHandle(i));
				if (asset)
				{
					textures[i] = asset->Texture;
					width = asset->Texture->Desc().Width;
					height = asset->Texture->Desc().Height;
					format = asset->Texture->Desc().Format;
				}
			}

			if (std::all_of(textures.begin(), textures.end(), [](const RefPtr<BFTexture>& tex) { return !tex; }))
			{
				return;
			};


			BFTextureDesc desc;
			desc.Width = width;
			desc.Height = height;
			desc.ArraySize = 6;
			desc.Format = format;
			desc.ViewTypes = BFTextureDesc::ViewType::ShaderResource;
			desc.Type = BFTextureType::Cubemap;
			desc.DebugName = "SkyboxCubemap";
			desc.UploadData.CubemapFacesSource= textures;

			m_skyboxTexture = MakeRef<BFTexture>(desc);
		}
		else
		{
			TextureAsset* asset = Application::Get().GetAssetManager().Resolve<TextureAsset>(component.GetTextureHandleHDRI());
			if (!asset || !asset->Texture)
			{
				return;
			}

			const uint32_t cubemapSize = asset->Texture->Desc().Width;
			BFTextureDesc desc;
			desc.Width = cubemapSize;
			desc.Height = cubemapSize;
			desc.ArraySize = 6;
			desc.Format = asset->Texture->Desc().Format;
			desc.ViewTypes = BFTextureDesc::ViewType::ShaderResource | BFTextureDesc::ViewType::UnorderedAccess;
			desc.Type = BFTextureType::Cubemap;
			desc.DebugName = "SkyboxCubemap";
			m_skyboxTexture = MakeRef<BFTexture>(desc);


			D3D12CommandList list;

			list.StartComputePass("EquirectangularToCubemap");

			m_skyboxTexture->Resource()->Transition(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			BFComputePSOInfo pso;
			pso.ComputeShader = BFShaderCache::GetOrCreate(L"assets/Shaders/EquirectangularToCubemap_cs.hlsl", ShaderType::Compute);

			TextureAsset* hdri = Application::Get().GetAssetManager().Resolve<TextureAsset>(component.GetTextureHandleHDRI());

			BFSampler smp;
			ShaderVariables()
				.Add(hdri->Texture->SRV().View())
				.Add(m_skyboxTexture->UAV().View())
				.Add(smp.View())
				.Add(cubemapSize)
				.Submit(list, true);

			list.SetComputePSO(pso);
			const uint32_t groupsX = (cubemapSize + 7) / 8;
			const uint32_t groupsY = (cubemapSize + 7) / 8;
			list.Dispatch(groupsX, groupsY, 6);

			m_skyboxTexture->Resource()->Transition(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			list.EndComputePass();

			list.Close();
			D3D12API()->Queue(QueueType::Direct)->Execute(list);
			D3D12API()->Queue(QueueType::Direct)->WaitForFence();

		}
	}

	void SkyboxRenderPipelineStage::UnloadSkybox()
	{
		m_skyboxTexture.reset();
	}

	void SkyboxRenderPipelineStage::OnRecordPass(const ViewportRenderEvent& ev)
	{
		if (!m_skyboxTexture)
		{
			return;
		}

		GraphBuilder& builder = ev.Builder;
		Viewport& viewport = ev.Viewport;

		struct SkyboxPassParams
		{
		};

		SkyboxPassParams* params = builder.AllocParameters<SkyboxPassParams>();

		ForwardRenderer* forwardParams = builder.GetPassData<SkyboxPassParams, ForwardRenderer>();

		builder.AddPass<SkyboxPassParams>("Skybox",
			[forwardParams, viewport, this](const SkyboxPassParams& params, D3D12CommandList& list)
			{
				BF_PROFILE_EVENT_DYNAMIC("Skybox pass");

				RasterPassStartInfo info;
				info.RenderTarget = &viewport.GetRenderTarget();
				info.DepthStencil = &viewport.GetDepthStencil();
				info.RenderTargetLoadOp = LoadOP::Load;
				info.DepthStencilLoadOp = LoadOP::Load;
				list.StartRenderPass(info, "Skybox Pass");

				list.SetViewport(RenderViewport::FromTexture(viewport.GetRenderTarget()));

				BFGraphicsPSOInfo pso;
				pso.Rasterizer.CullMode = D3D12_CULL_MODE_BACK;
				pso.DepthStencil.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
				pso.DepthStencil.WriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
				pso.DepthStencil.EnableDepth = true;
				pso.PixelShader = BFShaderCache::GetOrCreate(L"assets/Shaders/Skybox_frag.hlsl", ShaderType::Pixel);
				pso.VertexShader = BFShaderCache::GetOrCreate(L"assets/Shaders/Fullscreen_vert.hlsl", ShaderType::Vertex);
				list.SetGraphicsPSO(pso);

				BFSampler sampler;

				ShaderVariables()
					.Add(viewport.Uniforms->GetView(HASH("InverseCameraData"))->View())
					.Add(sampler.View())
					.Add(m_skyboxTexture->SRV().View())
					.Submit(list);

				list.DrawInstanced(6, 1, 0, 0);

				list.EndRenderPass();
			});
	}
}