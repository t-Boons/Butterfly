#pragma once
#include "Renderer/D3D12Texture.hpp"
#include "stbimage/stb_image.h"
#include "Scene/Registry/SkyboxComponent.hpp"

namespace Butterfly
{
	class Skybox
	{
	public:
		void LoadSkybox(const SkyboxComponent& component)
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

		void DeleteSkybox()
		{
			m_skyboxTexture.reset();
		}

		void SkyboxPass(const ViewportRenderEvent& ev)
		{
			if(!m_skyboxTexture)
				return;

			GraphBuilder& builder = ev.Builder;
			Viewport& viewport = ev.Viewport;

			struct SkyboxPassParams
			{
			};

			struct BufferIndices
			{
				int cameraUniforms;
				int skyboxSampler;
				int skyboxTexture;
			};

			struct SkyboxCameraData
			{
				glm::mat4 invView;
				glm::mat4 invProjection;
			};

			SkyboxPassParams* params = builder.AllocParameters<SkyboxPassParams>();

			ForwardRenderer* forwardParams = builder.GetPassData<SkyboxPassParams, ForwardRenderer>();

			builder.AddPass<SkyboxPassParams>("Skybox",
				[forwardParams, viewport, this](const SkyboxPassParams& params, D3D12CommandList& list)
				{
					BF_PROFILE_EVENT_DYNAMIC("Skybox pass");

					// Default Init stuff.
					list.List()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

					GraphicsCommands::SetRenderTargets(list, { viewport.RenderTarget.get()}, forwardParams->DepthStencil->Resource().get());

					GraphicsCommands::SetFullscreenViewportAndRect(list, viewport.RenderTarget->Width(), viewport.RenderTarget->Height());

					BFPipelineBuilder psoBuilder;
					psoBuilder.PrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
					psoBuilder.RenderTargetFormats({ DXGI_FORMAT_R8G8B8A8_UNORM });
					psoBuilder.DepthStencilFormat({ DXGI_FORMAT_D24_UNORM_S8_UINT });
					psoBuilder.VertexShader(BFShaderCache::GetOrCreate(L"assets/Shaders/Skybox_vert.hlsl", ShaderType::Vertex));
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

					list.List()->DrawInstanced(6, 1, 0, 0);
				});
		}

	private:
		RefPtr<BFTexture> m_skyboxTexture;
	};
}