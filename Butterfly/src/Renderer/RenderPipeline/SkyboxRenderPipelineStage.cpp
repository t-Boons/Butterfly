#pragma once
#include "Renderer/RenderPipeline/SkyboxRenderPipelineStage.hpp"
#include "Renderer/RenderPipeline/RenderPipeline.hpp"
#include "Renderer/Renderer.hpp"
#include "Renderer/D3D12Sampler.hpp"
#include "Renderer/D3D12/D3D12CommandQueue.hpp"
#include "Renderer/RenderData.hpp"

namespace Butterfly
{
	void SkyboxRenderPipelineStage::Update(const SkyboxComponent* component)
	{
		if (component)
		{
			if (static_cast<int>(component->m_version) != m_skyboxVersion)
			{
				m_skyboxVersion = component->m_version;
				LoadSkybox(*component);
			}
		}
		else
		{
			m_skyboxVersion = -1;
			UnloadSkybox();
		}
	}

	void SkyboxRenderPipelineStage::LoadSkybox(const SkyboxComponent& component)
	{
		uint32_t width = 0, height = 0;
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
		if (component.GetType() == SkyboxType::Cubemap)
		{
			std::array<Texture*, 6> textures{};
			for (uint32_t i = 0; i < 6; ++i)
			{
				TextureAsset* asset = Application::Get().GetAssetManager().Resolve<TextureAsset>(component.GetTextureHandle(i));
				if (asset)
				{
					if(asset->Texture)
					{
						textures[i] = asset->Texture.get();
						width = asset->Texture->Width();
						height = asset->Texture->Height();
						format = asset->Texture->Format();
					}
				}
			}

			if (std::all_of(textures.begin(), textures.end(), [](const Texture* tex) { return !tex; }))
			{
				BF_CORE_LOG_WARN("No valid textures found for skybox cubemap. Skipping load & Unloading.");
				UnloadSkybox();
				return;
			};


			TextureDesc desc;
			desc.Width = width;
			desc.Height = height;
			desc.ArraySize = 6;
			desc.NumMips = BF_ALL;
			desc.Format = format;
			desc.ViewTypes = TextureDesc::ViewType::ShaderResource;
			desc.Dimension = TextureDimension::Texture2D;
			desc.DebugName = "SkyboxCubemap";

			m_skyboxTexture = MakeRef<Texture>(desc);
			GraphicsCommands::UploadTextureCubeResource(*m_skyboxTexture, textures);

			D3D12CommandList list;
			for (uint32_t i = 0; i < 6; ++i)
			{
				GraphicsCommands::GenerateMips(list, *m_skyboxTexture, i);
			}

			list.Close();
			D3D12API()->Queue(QueueType::Direct)->Execute(list);
			D3D12API()->Queue(QueueType::Direct)->WaitForFence();
		}
		else
		{
			TextureAsset* asset = Application::Get().GetAssetManager().Resolve<TextureAsset>(component.GetTextureHandleHDRI());
			if (!asset || !asset->Texture)
			{
				return;
			}

			const uint32_t cubemapSize = asset->Texture->Height();
			TextureDesc desc;
			desc.Width = cubemapSize;
			desc.Height = cubemapSize;
			desc.ArraySize = 6;
			desc.NumMips = BF_ALL;
			desc.Format = asset->Texture->Format();
			desc.ViewTypes = TextureDesc::ViewType::ShaderResource | TextureDesc::ViewType::UnorderedAccess;
			desc.DebugName = "SkyboxCubemap";
			m_skyboxTexture = MakeRef<Texture>(desc);

			format = asset->Texture->Format();

			D3D12CommandList list;
			GraphicsCommands::EquirectangularToCubemap(list, *asset->Texture, *m_skyboxTexture);

			for (uint32_t i = 0; i < 6; ++i)
			{
				GraphicsCommands::GenerateMips(list, *m_skyboxTexture, i);
			}

			list.Close();
			D3D12API()->Queue(QueueType::Direct)->Execute(list);
			D3D12API()->Queue(QueueType::Direct)->WaitForFence();			
		}

		if(format != DXGI_FORMAT_UNKNOWN)
		{
			TextureDesc irradianceDesc;
			irradianceDesc.Format = format;
			irradianceDesc.Dimension = TextureDimension::Texture2D;
			irradianceDesc.Width = 32;
			irradianceDesc.Height = 32;
			irradianceDesc.ArraySize = 6;
			irradianceDesc.NumMips = 1;
			irradianceDesc.ViewTypes = TextureDesc::ShaderResource | TextureDesc::UnorderedAccess;
			irradianceDesc.DebugName = "SkyboxIrradiance";
			m_irradianceTexture = MakeRef<Texture>(irradianceDesc);

			GraphicsCommands::GenerateIrradiance(*m_skyboxTexture, *m_irradianceTexture, 32);
		}
	}

	void SkyboxRenderPipelineStage::UnloadSkybox()
	{
		m_skyboxTexture.reset();
		m_irradianceTexture.reset();
	}

	void SkyboxRenderPipelineStage::OnRecordPass(const ViewportRenderEvent& ev)
	{
		if (!m_skyboxTexture)
		{
			return;
		}

		GraphBuilder& builder = ev.Builder;
		Viewport& viewport = ev.Viewport;

		SkyboxPassParams* params = builder.AllocParameters<SkyboxPassParams>();
		params->SkyboxTexture = m_skyboxTexture.get();
		params->IrradianceTexture = m_irradianceTexture.get();

		builder.AddPass<SkyboxPassParams>("Skybox",
			[viewport, this](const SkyboxPassParams& params, D3D12CommandList& list)
			{
				BF_PROFILE_EVENT_DYNAMIC("Skybox pass");

				RasterPassStartInfo info;
				info.RenderTarget = &viewport.GetRenderTarget();
				info.DepthStencil = &viewport.GetDepthStencil();
				info.RenderTargetLoadOp = LoadOP::Load;
				info.DepthStencilLoadOp = LoadOP::Load;
				list.StartRenderPass(info, "Skybox Pass");

				list.SetViewport(RenderViewport::FromTexture(viewport.GetRenderTarget()));

				GraphicsPSOInfo pso;
				pso.Rasterizer.CullMode = D3D12_CULL_MODE_BACK;
				pso.DepthStencil.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
				pso.DepthStencil.WriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
				pso.DepthStencil.EnableDepth = true;
				pso.PixelShader = ShaderCache::GetOrCreate("Skybox_frag.hlsl");
				pso.VertexShader = ShaderCache::GetOrCreate("Fullscreen_vert.hlsl");
				list.SetGraphicsPSO(pso);

				Sampler& sampler = SamplerCache::GetOrCreate(SamplerDesc::LinearWrap());

				ShaderVariableBuilder(list)
					.Add(viewport.GetRenderData().CameraDataBuffer->CBV())
					.Add(sampler)
					.Add(m_skyboxTexture->SRV(SRVTextureViewInfo::Cubemap()))
					.Submit();

				list.DrawInstanced(6, 1, 0, 0);

				list.EndRenderPass();
			});
	}
}