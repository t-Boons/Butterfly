#include "EditorViewport/SceneViewport.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "ImGuizmo/ImGuizmo.h"
#include "Core/EditorApplication.hpp"
#include "EditorViewport/EditorViewport.hpp"

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

		InverseCameraData data;
		data.InverseView = glm::transpose(glm::inverse(m_spectatorCam.GetCamera()->ViewMatrix()));
		data.InverseProjection = glm::transpose(glm::inverse(m_spectatorCam.GetCamera()->ProjectionMatrix()));

		event.Viewport.Uniforms->GetOrCreateView(sizeof(InverseCameraData), HASH("InverseCameraData"));
		event.Viewport.Uniforms->Write(&data, sizeof(InverseCameraData), HASH("InverseCameraData"));
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
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && m_objectPickerReadback->ReadPixel(relativePos, readbackID) && !ImGuizmo::IsOver())
		{
			if (readbackID > 0)
			{
				// We do -1 because the rendred readbackID increments the entity count for entity 0.
				// Thus entity 0 is entity 1
				const uint32_t sceneEntityID = readbackID - 1; 

				EditorApplication::Get().GetEditorViewport().m_selectedEntity = Entity(&Application::Get().GetScene().GetEntityRegistry(), static_cast<entt::entity>(sceneEntityID));
				BF_LOG_INFO("Selected entity: %u", sceneEntityID);
			}
			else
			{
				EditorApplication::Get().GetEditorViewport().m_selectedEntity = Entity();
			}
		}


		// Viewport rendering.
		if (m_viewportHandle.Valid())
		{
			Application::Get().GetRenderer().ImGUIImage(m_viewportHandle);
		}

		static ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;

		if (ImGui::IsKeyPressed(ImGuiKey_W))
			currentGizmoOperation = ImGuizmo::TRANSLATE;
		if (ImGui::IsKeyPressed(ImGuiKey_E))
			currentGizmoOperation = ImGuizmo::ROTATE;
		if (ImGui::IsKeyPressed(ImGuiKey_R))
			currentGizmoOperation = ImGuizmo::SCALE;

		if (EditorApplication().Get().GetEditorViewport().m_selectedEntity)
		{
			const ImVec2 size = ImGui::GetItemRectSize();
			const ImVec2 position = ImGui::GetItemRectMin();

			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect(position.x, position.y, size.x, size.y);

			TransformComponent& entityTransform = EditorApplication().Get().GetEditorViewport().m_selectedEntity.GetComponent<TransformComponent>();


			glm::mat4 changableMatrix = entityTransform.GetWorldMatrix();
			ImGuizmo::Manipulate(
				&m_spectatorCam.GetCamera()->ViewMatrix()[0][0],
				&m_spectatorCam.GetCamera()->ProjectionMatrix()[0][0],
				currentGizmoOperation,
				ImGuizmo::WORLD,
				&changableMatrix[0][0]
			);

			if (ImGuizmo::IsUsing())
			{
				entityTransform.SetWorldMatrix(changableMatrix);
			}
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
		desc.DebugName = "R32 Viewport objectpicker";
		params->RenderTarget = builder.CreateTransientTexture("R32 Viewport objectpicker", desc);

		BFTextureDesc desc2;
		desc2.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		desc2.Width = viewport.RenderTarget->Width();
		desc2.Height = viewport.RenderTarget->Height();
		desc2.Flags = BFTextureDesc::DepthStencilable;
		desc2.DebugName = "DepthStencil Viewport objectpicker";
		params->DepthStencil = builder.CreateTransientTexture("DepthStencil Viewport objectpicker", desc2);


		event.Builder.AddPass<ObjectPickerPassData>("RenderObjectPickerPass", [&](const ObjectPickerPassData& data, D3D12CommandList& list)
			{
				BFTexture& rt = *data.RenderTarget->Resource();

				// Default Init stuff.
				list.List()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				GraphicsCommands::SetRenderTargets(list, { &rt }, data.DepthStencil->Resource().get());

				GraphicsCommands::ClearDepthStencil(list, *data.DepthStencil->Resource());
				GraphicsCommands::ClearRenderTarget(list, rt, { 0.0f, 0.0f, 0.0f, 0.0f });

				GraphicsCommands::SetFullscreenViewportAndRect(list, rt.Width(), rt.Height());

				BFPipelineBuilder psoBuilder;
				psoBuilder.PrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
				psoBuilder.RenderTargetFormats({ DXGI_FORMAT_R32_UINT });
				psoBuilder.DepthStencilFormat({ DXGI_FORMAT_D24_UNORM_S8_UINT });
				psoBuilder.VertexShader(BFShaderCache::GetOrCreate(L"assets/Shaders/ObjectPicker_vert.hlsl", ShaderType::Vertex));
				psoBuilder.PixelShader(BFShaderCache::GetOrCreate(L"assets/Shaders/ObjectPicker_frag.hlsl", ShaderType::Pixel));
				psoBuilder.CullingMode(D3D12_CULL_MODE_BACK);

				list.List()->SetPipelineState(psoBuilder.Create().GetHW());

				uint32_t entityRenderIndex = 0;
				auto view = Application::Get().GetScene().GetEntityRegistry().view<TransformComponent, MeshRendererComponent>();
				for (auto [entity, transform, meshRenderer] : view.each())
				{
					if (!meshRenderer.GetMeshHandle())
					{
						continue;
					}

					AssetManager& as = Application::Get().GetAssetManager();
					MeshAsset* mesh = as.Resolve<MeshAsset>(meshRenderer.GetMeshHandle());
					ShaderVariables()
						.Add(mesh->GPUPositions->SRV().View())
						.Add(viewport.Uniforms->GetView(HASH("CameraData"))->View())
						.Add(viewport.ModelMatrices->SRV().View())
						.Add(entityRenderIndex) // Rendered entity index.
						.Add(static_cast<int>(entity)) // uint32_t Entity ID in registry.
						.Submit(list);

					entityRenderIndex++;

					list.List()->IASetIndexBuffer(&mesh->GPUIndices->IBV());
					list.List()->DrawIndexedInstanced(mesh->GPUIndices->NumElements(), 1, 0, 0, 0);
				}

				m_objectPickerReadback->ReadbackCopy(list, data.RenderTarget->Resource());
			});
	}
}