#include "ForwardSponza.hpp"
#include "../App.hpp"
#include "../Tools/Camera.hpp"

namespace GraphExamples
{
	void ForwardSponza::Init()
	{
		m_uniforms = ScopePtr<BFUniformBuffer>(new BFUniformBuffer(256, "UniformBuffer"));
		m_sponza = SponzaModel::Get();
	}

	void ForwardSponza::Record(Butterfly::GraphBuilder& builder, const Butterfly::Camera& camera)
	{
		using namespace Butterfly;

		struct UniformData
		{
			glm::mat4 ViewProjection;
			glm::mat4 InvViewProjection;
			glm::mat4 Model;
		};

		UniformData uniformData;
		uniformData.ViewProjection = camera.ViewProjectionMatrix();
		uniformData.Model = m_sponza->ModelMatrix;

		m_uniforms->Write(&uniformData, sizeof(uniformData));


		struct ForwardRenderer
		{
			BFRGTexture* DepthStencil;
		};

		ForwardRenderer* params = builder.AllocParameters<ForwardRenderer>();

		BFTextureDesc desc2;
		desc2.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		desc2.Width = App::CompositeTexture->Width();
		desc2.Height = App::CompositeTexture->Height();
		desc2.Flags = BFTextureDesc::DepthStencilable;
		params->DepthStencil = builder.CreateTransientTexture("DepthStencil Positions", desc2);

		builder.AddPass<ForwardRenderer>("Forward Sponza pass",
			[&](const ForwardRenderer& params, DX12CommandList& list)
			{
				BF_PROFILE_EVENT_DYNAMIC("Forward Sponza pass");

				BFTexture& rt = *App::CompositeTexture;

				// Default Init stuff.
				list.List()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				GraphicsCommands::SetRenderTargets(list, { &rt }, params.DepthStencil->Resource());

				GraphicsCommands::ClearDepthStencil(list, *params.DepthStencil->Resource());
				GraphicsCommands::ClearRenderTarget(list, rt, { 0.05f, 0.1f, 0.15f, 1.0f });

				GraphicsCommands::SetFullscreenViewportAndRect(list, App::CompositeTexture->Width(), App::CompositeTexture->Height());

				BFPipelineBuilder psoBuilder;
				psoBuilder.PrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
				psoBuilder.RenderTargetFormats({ DXGI_FORMAT_R8G8B8A8_UNORM });
				psoBuilder.DepthStencilFormat({ DXGI_FORMAT_D24_UNORM_S8_UINT });
				psoBuilder.VertexShader(BFShaderCache::GetOrCreate(L"assets/Shaders/Forward_vert.hlsl", ShaderType::Vertex));
				psoBuilder.PixelShader(BFShaderCache::GetOrCreate(L"assets/Shaders/Forward_frag.hlsl", ShaderType::Pixel));
				psoBuilder.CullingMode(D3D12_CULL_MODE_FRONT);

				list.List()->SetPipelineState(psoBuilder.Create().GetHW());

		
				BFSampler sampler;
				for (size_t i = 0; i < m_sponza->Meshes.size(); i++)
				{
					const size_t mesh = i;

					for (size_t j = 0; j < m_sponza->Meshes[mesh] + 1; j++)
					{
						const size_t primitive = j;

						const auto& positions = *m_sponza->Positions[mesh][primitive];
						const auto& normals = *m_sponza->Normals[mesh][primitive];
						const auto& texcoords = *m_sponza->TexCoords[mesh][primitive];
						const auto& indexBuffer = m_sponza->IndexBuffers[mesh][primitive];
						const auto& texture = m_sponza->Textures[m_sponza->TextureIndices[primitive]];

						ShaderVariables()
							.Add(positions.SRV().View())
							.Add(normals.SRV().View())
							.Add(texcoords.SRV().View())
							.Add(m_uniforms->CBV().View())
							.Add(sampler.View())
							.Add(texture->SRV().View())
							.Add(m_sponza->m_lights->SRV().View())
							.Submit(list);

						list.List()->IASetIndexBuffer(&indexBuffer->IBV());
						list.List()->DrawIndexedInstanced(indexBuffer->NumElements(), 1, 0, 0, 0);
					}
				}
			});
	}
}