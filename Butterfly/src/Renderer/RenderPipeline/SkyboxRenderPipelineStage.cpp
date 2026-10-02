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
		if (component.GetType() == SkyboxType::Cubemap)
		{

			std::array<RefPtr<BFTexture>, 6> textures;
			for (uint32_t i = 0; i < 6; ++i)
			{
				TextureAsset* asset = Application::Get().GetAssetManager().Resolve<TextureAsset>(component.GetTextureHandle(i));
				if (asset)
				{
					textures[i] = asset->Texture;
				}
			}

			if (std::all_of(textures.begin(), textures.end(), [](const RefPtr<BFTexture>& tex) { return !tex; }))
			{
				return;
			};

			m_skyboxTexture = BFTexture::CreateCubemap(textures);
		}
		else
		{
			TextureAsset* asset = Application::Get().GetAssetManager().Resolve<TextureAsset>(component.GetTextureHandleHDRI());
			if (!asset || !asset->Texture)
			{
				return;
			}

			BFTextureDesc desc;
			desc.Width = 2048;
			desc.Height = 2048;
			desc.ArraySize = 6;
			desc.Format = asset->Texture->Desc().Format;
			desc.Flags = BFTextureDesc::ShaderResource | BFTextureDesc::UnorderedAccess;
			desc.Type = BFTextureType::Cubemap;
			desc.DebugName = "SkyboxCubemap";
			m_skyboxTexture = BFTexture::CreateTextureForGPU(desc);



			D3D12CommandList list;
			m_skyboxTexture->Resource()->Transition(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			GraphicsCommands::SetBindlessDescriptorHeapsAndRootSignature(list);

			BFComputePipelineState pso;
			pso.ComputeShader = BFShaderCache::GetOrCreate(L"assets/Shaders/EquirectangularToCubemap_cs.hlsl", ShaderType::Compute);

			TextureAsset* hdri = Application::Get().GetAssetManager().Resolve<TextureAsset>(component.GetTextureHandleHDRI());

			BFSampler smp;
			ShaderVariables()
				.Add(hdri->Texture->SRV().View())
				.Add(m_skyboxTexture->UAV().View())
				.Add(smp.View())
				.Add(2048)
				.Submit(list, true);

			list.List()->SetPipelineState(BFPipelineStateCache::GetOrCreatePipeline(pso).GetHW());
			const uint32_t groupsX = (desc.Width + 7) / 8;
			const uint32_t groupsY = (desc.Width + 7) / 8;
			list.Dispatch(groupsX, groupsY, 6);

			m_skyboxTexture->Resource()->Transition(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

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

				// Default Init stuff.
				list.List()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				GraphicsCommands::SetRenderTargets(list, { &viewport.GetRenderTarget() }, &viewport.GetDepthStencil());

				GraphicsCommands::SetFullscreenViewportAndRect(list, viewport.GetRenderTarget().Width(), viewport.GetRenderTarget().Height());

				BFPipelineBuilder psoBuilder;
				psoBuilder.PrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
				psoBuilder.RenderTargetFormats({ DXGI_FORMAT_R8G8B8A8_UNORM });
				psoBuilder.DepthStencilFormat({ DXGI_FORMAT_D24_UNORM_S8_UINT });
				psoBuilder.VertexShader(BFShaderCache::GetOrCreate(L"assets/Shaders/Fullscreen_vert.hlsl", ShaderType::Vertex));
				psoBuilder.PixelShader(BFShaderCache::GetOrCreate(L"assets/Shaders/Skybox_frag.hlsl", ShaderType::Pixel));
				psoBuilder.CullingMode(D3D12_CULL_MODE_BACK);
				psoBuilder.DepthEnable(true);
				psoBuilder.DepthWriteMask(D3D12_DEPTH_WRITE_MASK_ZERO);
				psoBuilder.DepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);

				list.List()->SetPipelineState(psoBuilder.Create().GetHW());

				BFSampler sampler;

				ShaderVariables()
					.Add(viewport.Uniforms->GetView(HASH("InverseCameraData"))->View())
					.Add(sampler.View())
					.Add(m_skyboxTexture->SRV().View())
					.Submit(list);

				list.DrawInstanced(6, 1, 0, 0);
			});
	}
}