#include "DeferredSponza.hpp"
#include "../App.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

using namespace Butterfly;

namespace GraphExamples
{
	void DeferredSponza::Init()
	{
		m_uniforms = ScopePtr<BFUniformBuffer>(new BFUniformBuffer(512, "UniformBuffer"));
		m_sponza = SponzaModel::Get();
	}

	void DeferredSponza::Record(Butterfly::GraphBuilder& builder, const Butterfly::Camera& camera)
	{
		BF_PROFILE_EVENT();

		using namespace Butterfly;

		struct UniformData
		{
			glm::mat4 ViewProjection;
			glm::mat4 InverseView;
			glm::mat4 Model;
			glm::mat4 InverseProjection;
			glm::ivec2 Resolution;
		};

		UniformData uniformData;
		uniformData.ViewProjection = camera.ViewProjectionMatrix();
		uniformData.Model = m_sponza->ModelMatrix;
		uniformData.InverseView = glm::inverse(camera.ViewMatrix());
		uniformData.InverseProjection = glm::inverse(camera.ProjectionMatrix());
		uniformData.Resolution = { App::CompositeTexture->Width(), App::CompositeTexture->Height() };
		m_uniforms->Write(&uniformData, sizeof(uniformData));

		struct GBufferNormalsAndDepth
		{
			BFRGTexture* DepthStencil;
			BFRGTexture* OutputTexture;
		};

		{
			GBufferNormalsAndDepth* params = builder.AllocParameters<GBufferNormalsAndDepth>();

			BFTextureDesc desc;
			desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
			desc.Width = App::CompositeTexture->Width();
			desc.Height = App::CompositeTexture->Height();
			desc.Flags = BFTextureDesc::RenderTargettable | BFTextureDesc::ShaderResource;
			params->OutputTexture = builder.CreateTransientTexture({ desc, "Output Normals" });

			BFTextureDesc desc2;
			desc2.Format = DXGI_FORMAT_D32_FLOAT;
			desc2.Width = App::CompositeTexture->Width();
			desc2.Height = App::CompositeTexture->Height();
			desc2.Flags = BFTextureDesc::DepthStencilable;


			params->DepthStencil = builder.CreateTransientTexture({ desc2, "DepthStencil Normals" });

			builder.AddPass<GBufferNormalsAndDepth>("GBufferNormals pass",
				[&](const GBufferNormalsAndDepth& params, DX12CommandList& list)
				{
					BF_PROFILE_EVENT();

					BFTexture& rt = *params.OutputTexture->Resource();

					// Default Init stuff.
					list.List()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

					GraphicsCommands::SetRenderTargets(list, { &rt }, params.DepthStencil->Resource());

					GraphicsCommands::ClearDepthStencil(list, *params.DepthStencil->Resource());
					GraphicsCommands::ClearRenderTarget(list, rt);

					GraphicsCommands::SetFullscreenViewportAndRect(list, App::CompositeTexture->Width(), App::CompositeTexture->Height());

					BFPipelineBuilder psoBuilder;
					psoBuilder.PrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
					psoBuilder.RenderTargetFormats({ DXGI_FORMAT_R16G16B16A16_FLOAT });
					psoBuilder.DepthStencilFormat(DXGI_FORMAT_D32_FLOAT);
					psoBuilder.VertexShader(BFShaderCache::GetOrCreate(L"assets/Shaders/DeferredNormals_vert.hlsl", ShaderType::Vertex));
					psoBuilder.PixelShader(BFShaderCache::GetOrCreate(L"assets/Shaders/DeferredNormals_frag.hlsl", ShaderType::Pixel));
					psoBuilder.CullingMode(D3D12_CULL_MODE_FRONT);
					list.List()->SetPipelineState(psoBuilder.Create().GetHW());

					const auto& uniforms = *m_uniforms;

					for (size_t i = 0; i < m_sponza->Meshes.size(); i++)
					{
						const size_t mesh = i;

						for (size_t j = 0; j < m_sponza->Meshes[mesh] + 1; j++)
						{
							const size_t primitive = j;

							const auto& position = *m_sponza->Positions[mesh][primitive];
							const auto& normals = *m_sponza->Normals[mesh][primitive];
							const auto& indexBuffer = m_sponza->IndexBuffers[mesh][primitive];

							ShaderVariables()
								.Add(position.SRV().View())
								.Add(normals.SRV().View())
								.Add(uniforms.CBV().View())
								.Submit(list);

							list.List()->IASetIndexBuffer(&indexBuffer->IBV());
							list.List()->DrawIndexedInstanced(indexBuffer->NumElements(), 1, 0, 0, 0);
						}
					}
					params.OutputTexture->Resource()->Resource()->Transition(list, D3D12_RESOURCE_STATE_GENERIC_READ);
				});
		}

		struct GBufferAlbedo
		{
			BFRGTexture* DepthStencil;
			BFRGTexture* OutputTexture;
		};

		{
			GBufferAlbedo* params = builder.AllocParameters<GBufferAlbedo>();

			BFTextureDesc desc;
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.Width = App::CompositeTexture->Width();
			desc.Height = App::CompositeTexture->Height();
			desc.Flags = BFTextureDesc::RenderTargettable | BFTextureDesc::ShaderResource;

			params->OutputTexture = builder.CreateTransientTexture({ desc, "Output Albedo" });

			BFTextureDesc desc2;
			desc2.Format = DXGI_FORMAT_D32_FLOAT;
			desc2.Width = App::CompositeTexture->Width();
			desc2.Height = App::CompositeTexture->Height();
			desc2.Flags = BFTextureDesc::DepthStencilable;
			params->DepthStencil = builder.CreateTransientTexture({ desc2, "DepthStencil Albedo" });

			builder.AddPass<GBufferAlbedo>("GBufferAlbedo pass",
				[&](const GBufferAlbedo& params, DX12CommandList& list)
				{
					BF_PROFILE_EVENT();

					BFTexture& rt = *params.OutputTexture->Resource();

					// Default Init stuff.
					list.List()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

					GraphicsCommands::SetRenderTargets(list, { &rt }, params.DepthStencil->Resource());

					GraphicsCommands::ClearDepthStencil(list, *params.DepthStencil->Resource());
					GraphicsCommands::ClearRenderTarget(list, rt);

					GraphicsCommands::SetFullscreenViewportAndRect(list, App::CompositeTexture->Width(), App::CompositeTexture->Height());


					BFPipelineBuilder psoBuilder;
					psoBuilder.PrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
					psoBuilder.RenderTargetFormats({ DXGI_FORMAT_R8G8B8A8_UNORM });
					psoBuilder.DepthStencilFormat(DXGI_FORMAT_D32_FLOAT);
					psoBuilder.VertexShader(BFShaderCache::GetOrCreate(L"assets/Shaders/DeferredAlbedo_vert.hlsl", ShaderType::Vertex));
					psoBuilder.PixelShader(BFShaderCache::GetOrCreate(L"assets/Shaders/DeferredAlbedo_frag.hlsl", ShaderType::Pixel));
					psoBuilder.CullingMode(D3D12_CULL_MODE_FRONT);

					list.List()->SetPipelineState(psoBuilder.Create().GetHW());

					const auto& uniforms = *m_uniforms;


					BFSampler sampler;
					for (size_t i = 0; i < m_sponza->Meshes.size(); i++)
					{
						const size_t mesh = i;

						for (size_t j = 0; j < m_sponza->Meshes[mesh] + 1; j++)
						{
							const size_t primitive = j;

							const auto& position = *m_sponza->Positions[mesh][primitive];
							const auto& texcoords = *m_sponza->TexCoords[mesh][primitive];
							const auto& indexBuffer = m_sponza->IndexBuffers[mesh][primitive];
							const auto& texture = m_sponza->Textures[m_sponza->TextureIndices[primitive]];

							ShaderVariables()
								.Add(position.SRV().View())
								.Add(texcoords.SRV().View())
								.Add(uniforms.CBV().View())
								.Add(sampler.View())
								.Add(texture->SRV().View())
								.Submit(list);
							
							list.List()->IASetIndexBuffer(&indexBuffer->IBV());
							list.List()->DrawIndexedInstanced(indexBuffer->NumElements(), 1, 0, 0, 0);
						}
					}
					params.OutputTexture->Resource()->Resource()->Transition(list, D3D12_RESOURCE_STATE_GENERIC_READ);
					m_sponza->m_lights->DXResource().Transition(list, D3D12_RESOURCE_STATE_GENERIC_READ);
				});
		}

		struct GBufferLight
		{
			BFRGTexture* NormalXYZDepthATexture;
			BFRGTexture* AlbedoTexture;
		};

		{
			GBufferLight* params = builder.AllocParameters<GBufferLight>();

			params->NormalXYZDepthATexture = builder.GetPassData<GBufferLight, GBufferNormalsAndDepth>()->OutputTexture;
			params->AlbedoTexture = builder.GetPassData<GBufferLight, GBufferAlbedo>()->OutputTexture;

			builder.AddPass<GBufferLight>("GBuffer lighting pass",
				[&](const GBufferLight& params, DX12CommandList& list)
				{
					BF_PROFILE_EVENT();

					//BFTexture& rt = *params.OutputTexture->Resource();
					BFTexture& rt = *App::CompositeTexture;

					// Default Init stuff.
					list.List()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

					GraphicsCommands::SetRenderTargets(list, { &rt }, nullptr);
					GraphicsCommands::ClearRenderTarget(list, rt);
					GraphicsCommands::SetFullscreenViewportAndRect(list, App::CompositeTexture->Width(), App::CompositeTexture->Height());

					BFPipelineBuilder psoBuilder;
					psoBuilder.PrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
					psoBuilder.RenderTargetFormats({ DXGI_FORMAT_R8G8B8A8_UNORM });
					psoBuilder.VertexShader(BFShaderCache::GetOrCreate(L"assets/Shaders/DeferredLighting_vert.hlsl", ShaderType::Vertex));
					psoBuilder.PixelShader(BFShaderCache::GetOrCreate(L"assets/Shaders/DeferredLighting_frag.hlsl", ShaderType::Pixel));
					psoBuilder.CullingMode(D3D12_CULL_MODE_BACK);
					psoBuilder.EnableBlending();

					list.List()->SetPipelineState(psoBuilder.Create().GetHW());


					ShaderVariables()
						.Add(Sphere::Get()->m_positions->SRV().View())
						.Add(params.NormalXYZDepthATexture->Resource()->SRV().View())
						.Add(m_uniforms->CBV().View())
						.Add(BFSampler().View())
						.Add(m_sponza->m_lights->SRV().View())
						.Add(params.AlbedoTexture->Resource()->SRV().View())
						.Submit(list);


					list.List()->IASetIndexBuffer(&Sphere::Get()->m_indices->IBV());
					list.List()->DrawIndexedInstanced(Sphere::Get()->m_indices->NumElements(), SponzaModel::Get()->m_lights->NumBytes() / sizeof(PointLight), 0, 0, 0);
				});
		}
	}
}