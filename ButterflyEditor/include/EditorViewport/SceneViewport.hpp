#pragma once
#include "Butterfly.hpp"
#include "Tools/Camera.hpp"

namespace Butterfly
{
	class SceneViewport
	{
		RefPtr<BFTextureReadback> m_objectPickerReadback;

		SceneViewport()
		{
			Application::Get().GetRenderer().OnRecordRenderPasses.Subscribe(BF_BIND_FUNC_PARAM(&SceneViewport::RenderObjectPicker));
		}


		void RenderObjectPicker(const RecordRenderPassEvent& event)
		{
			GraphBuilder& builder = ev.Builder;
			Viewport& viewport = ev.Viewport;

			struct ObjectPickerPassData
			{ 
				BFTexture* DepthStencil;
				BFTexture* RenderTarget;
			};

			ObjectPickerPassData* params = builder.AllocParameters<ObjectPickerPassData>();

			BFTextureDesc desc;
			desc.Format = DXGI_FORMAT_R32_UINT;
			desc.Width = viewport.RenderTarget->Width();
			desc.Height = viewport.RenderTarget->Height();
			desc.Flags = BFTextureDesc::RenderTargettable;
			params->RenderTarget = builder.CreateTransientTexture("R32 Viewport objectpicker " + std::to_string(viewport.ViewportIndex), desc);

			BFTextureDesc desc2;
			desc2.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			desc2.Width = viewport.RenderTarget->Width();
			desc2.Height = viewport.RenderTarget->Height();
			desc2.Flags = BFTextureDesc::DepthStencilable;
			params->DepthStencil = builder.CreateTransientTexture("DepthStencil Viewport objectpicker " + std::to_string(viewport.ViewportIndex), desc2);

			event.Builder.AddPass<ObjectPickerPassData>("RenderObjectPickerPass", [&](const ObjectPickerPassData& data, D3D12CommandList& list)
				{

					BFTexture& rt = *params.RenderTarget;

					// Default Init stuff.
					list.List()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

					GraphicsCommands::SetRenderTargets(list, { &rt }, params.DepthStencil->Resource().get());

					GraphicsCommands::ClearDepthStencil(list, *params.DepthStencil->Resource());
					GraphicsCommands::ClearRenderTarget(list, rt, { 0.05f, 0.1f, 0.15f, 1.0f });

					GraphicsCommands::SetFullscreenViewportAndRect(list, rt.Width(), rt.Height());

					BFPipelineBuilder psoBuilder;
					psoBuilder.PrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
					psoBuilder.RenderTargetFormats({ DXGI_FORMAT_R8G8B8A8_UNORM });
					psoBuilder.DepthStencilFormat({ DXGI_FORMAT_D24_UNORM_S8_UINT });
					psoBuilder.VertexShader(BFShaderCache::GetOrCreate(L"assets/Shaders/ObjectPicker_vert.hlsl", ShaderType::Vertex));
					psoBuilder.PixelShader(BFShaderCache::GetOrCreate(L"assets/Shaders/ObjectPicker_frag.hlsl", ShaderType::Pixel));
					psoBuilder.CullingMode(D3D12_CULL_MODE_FRONT);

					list.List()->SetPipelineState(psoBuilder.Create().GetHW());


					uint32_t entityIndex = 0;
					auto view = Application::Get().GetScene().GetEntityRegistry().view<TransformComponent, MeshRendererComponent>();

					for (auto [entity, transform, meshRenderer] : view.each())
					{
						if (!meshRenderer.ContainsMesh())
						{
							continue;
						}

						struct UniformCameraData
						{
							glm::mat4 ViewProjection;
						};

						UniformCameraData cameraData;
						cameraData.ViewProjection = Application::Get().GetBlackboard().Get<Camera>("ViewCamera")->ViewProjectionMatrix();

						viewport.Uniforms->Write(&cameraData, sizeof(UniformCameraData), viewport.UniformCameraDataViewIndex);

						const glm::mat4 model = transform.GetMatrix();
						viewport.ModelMatrices->Write(&model, sizeof(glm::mat4), entityIndex * sizeof(glm::mat4));

						AssetManager& as = Application::Get().GetAssetManager();
						MeshAsset* mesh = as.Resolve<MeshAsset>(meshRenderer.MeshHandle);
						ShaderVariables()
							.Add(mesh->GPUPositions->SRV().View())
							.Add(viewport.Uniforms->GetView(viewport.UniformCameraDataViewIndex)->View())
							.Add(viewport.ModelMatrices->SRV().View())
							.Add(entity) // uint32_t Entity ID in registry.
							.Submit(list);

						list.List()->IASetIndexBuffer(&mesh->GPUIndices->IBV());
						list.List()->DrawIndexedInstanced(mesh->GPUIndices->NumElements(), 1, 0, 0, 0);

						entityIndex++;
				});
		}
	};
}