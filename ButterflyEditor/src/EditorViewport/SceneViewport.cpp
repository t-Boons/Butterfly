#include "EditorViewport/SceneViewport.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace Butterfly
{
	struct ObjectPickerPassData
	{
		BFRGTexture* DepthStencil;
		BFRGTexture* RenderTarget;
	};

	SceneViewport::SceneViewport()
	{
		m_viewportHandle = Application::Get().GetRenderer().AddViewport();

		m_objectPickerReadback = MakeRef<BFTextureReadback>();

		m_viewportResizeReceiver.Subscribe(Application::Get().GetRenderer().GetViewportEvents(m_viewportHandle).OnResize, BF_BIND_FUNC_PARAM(&SceneViewport::OnResize));
		m_viewportRenderReceiver.Subscribe(Application::Get().GetRenderer().GetViewportEvents(m_viewportHandle).OnRender, BF_BIND_FUNC_PARAM(&SceneViewport::RenderObjectPicker));
		m_viewportPrerenderReceiver.Subscribe(Application::Get().GetRenderer().GetViewportEvents(m_viewportHandle).OnPreRender, BF_BIND_FUNC_PARAM(&SceneViewport::OnPrerender));
	}
	
	SceneViewport::~SceneViewport()
	{
		Application::Get().GetRenderer().RemoveViewport(m_viewportHandle);
	}

	void SceneViewport::OnTick()
	{
		m_spectatorCam.Tick(Application::Get().GetInput(), Application::Get().GetTime().DeltaTime());
	}

	void SceneViewport::OnResize(const ViewportResizeEvent& event)
	{
		auto p = m_spectatorCam.GetCamera()->Projection();
		p.AspectRatio = static_cast<float>(event.Size.x) / static_cast<float>(event.Size.y);
		m_spectatorCam.GetCamera()->SetProjection(p);
	}

	void SceneViewport::OnPrerender(const ViewportPrerenderEvent& event)
	{
		CameraData cameraData;
		cameraData.ViewProjection = m_spectatorCam.GetCamera()->ViewProjectionMatrix();

		event.Viewport.Uniforms->GetOrCreateView(sizeof(CameraData), HASH("CameraData"));
		event.Viewport.Uniforms->Write(&cameraData, sizeof(CameraData), HASH("CameraData"));
	}

	void SceneViewport::OnRenderImGUI()
	{
		ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_NoWindowMenuButton;
		ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), dockspaceFlags);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground);

		// Object selection.
		const glm::ivec2 mousePos = glm::ivec2(ImGui::GetMousePos().x, ImGui::GetMousePos().y);
		const glm::ivec2 contentPos = glm::ivec2(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y) + glm::ivec2(ImGui::GetWindowContentRegionMin().x, ImGui::GetWindowContentRegionMin().y);
		const glm::ivec2 relativePos = mousePos - contentPos;
		uint32_t readbackID = 0;
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && m_objectPickerReadback->ReadPixel(relativePos, readbackID))
		{

			if (readbackID > 0)
			{
				// We do -1 because the rendred readbackID increments the entity count for entity 0.
				// Thus entity 0 is entity 1
				const uint32_t sceneEntityID = readbackID - 1; 
				m_selectedEntity = static_cast<entt::entity>(sceneEntityID);
				BF_LOG_INFO("Selected entity: %u", sceneEntityID);
			}
			else
			{
				m_selectedEntity = entt::null;
			}
		}


		// Viewport rendering.
		if (m_viewportHandle.Valid())
		{
			Application::Get().GetRenderer().ImGUIImage(m_viewportHandle);
		}

		ImGui::PopStyleVar(2);
		ImGui::End();
	}

	void SceneViewport::RenderObjectPicker(const ViewportRenderEvent& event)
	{
		GraphBuilder& builder = event.Builder;
		Viewport& viewport = event.Viewport;

		ObjectPickerPassData* params = builder.AllocParameters<ObjectPickerPassData>();

		BFTextureDesc desc;
		desc.Format = DXGI_FORMAT_R32_UINT;
		desc.Width = viewport.RenderTarget->Width();
		desc.Height = viewport.RenderTarget->Height();
		desc.Flags = BFTextureDesc::RenderTargettable;
		params->RenderTarget = builder.CreateTransientTexture("R32 Viewport objectpicker", desc);

		BFTextureDesc desc2;
		desc2.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		desc2.Width = viewport.RenderTarget->Width();
		desc2.Height = viewport.RenderTarget->Height();
		desc2.Flags = BFTextureDesc::DepthStencilable;
		params->DepthStencil = builder.CreateTransientTexture("DepthStencil Viewport objectpicker", desc2);


		event.Builder.AddPass<ObjectPickerPassData>("RenderObjectPickerPass", [&](const ObjectPickerPassData& data, D3D12CommandList& list)
			{
				BFTexture& rt = *data.RenderTarget->Resource();

				// Default Init stuff.
				list.List()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				GraphicsCommands::SetRenderTargets(list, { &rt }, data.DepthStencil->Resource().get());

				GraphicsCommands::ClearDepthStencil(list, *data.DepthStencil->Resource());
				GraphicsCommands::ClearRenderTarget(list, rt, { 0.05f, 0.1f, 0.15f, 1.0f });

				GraphicsCommands::SetFullscreenViewportAndRect(list, rt.Width(), rt.Height());

				BFPipelineBuilder psoBuilder;
				psoBuilder.PrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
				psoBuilder.RenderTargetFormats({ DXGI_FORMAT_R32_UINT });
				psoBuilder.DepthStencilFormat({ DXGI_FORMAT_D24_UNORM_S8_UINT });
				psoBuilder.VertexShader(BFShaderCache::GetOrCreate(L"assets/Shaders/ObjectPicker_vert.hlsl", ShaderType::Vertex));
				psoBuilder.PixelShader(BFShaderCache::GetOrCreate(L"assets/Shaders/ObjectPicker_frag.hlsl", ShaderType::Pixel));
				psoBuilder.CullingMode(D3D12_CULL_MODE_FRONT);

				list.List()->SetPipelineState(psoBuilder.Create().GetHW());

				auto view = Application::Get().GetScene().GetEntityRegistry().view<TransformComponent, MeshRendererComponent>();

				for (auto [entity, transform, meshRenderer] : view.each())
				{
					if (!meshRenderer.ContainsMesh())
					{
						continue;
					}

					AssetManager& as = Application::Get().GetAssetManager();
					MeshAsset* mesh = as.Resolve<MeshAsset>(meshRenderer.MeshHandle);
					ShaderVariables()
						.Add(mesh->GPUPositions->SRV().View())
						.Add(viewport.Uniforms->GetView(HASH("CameraData"))->View())
						.Add(viewport.ModelMatrices->SRV().View())
						.Add(static_cast<int>(entity)) // uint32_t Entity ID in registry.
						.Submit(list);

					list.List()->IASetIndexBuffer(&mesh->GPUIndices->IBV());
					list.List()->DrawIndexedInstanced(mesh->GPUIndices->NumElements(), 1, 0, 0, 0);
				}

				m_objectPickerReadback->ReadbackCopy(list, data.RenderTarget->Resource());
			});
	}
}