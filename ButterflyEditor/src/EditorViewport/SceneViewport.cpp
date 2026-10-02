#include "EditorViewport/SceneViewport.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "Core/EditorApplication.hpp"
#include "EditorViewport/EditorViewport.hpp"
#include "Core/DebugRenderer.hpp"
#include "ImGUI/FontAwesomeIcons.hpp"

namespace Butterfly
{
	struct ObjectPickerPassData
	{
		BFRGTexture* DepthStencil;
		BFRGTexture* RenderTarget;
	};


	glm::ivec2 WorldToViewport(const glm::vec3& worldPos, const glm::mat4& viewProjection, const glm::ivec2& viewportSize)
	{
		glm::vec4 clipSpacePos = viewProjection * glm::vec4(worldPos, 1.0f);
		glm::vec3 ndcSpacePos = glm::vec3(clipSpacePos) / clipSpacePos.w;
		glm::vec2 viewportPos;
		viewportPos.x = (ndcSpacePos.x * 0.5f + 0.5f) * viewportSize.x;
		viewportPos.y = (1.0f - (ndcSpacePos.y * 0.5f + 0.5f)) * viewportSize.y;
		return glm::ivec2(viewportPos);
	}

	class ObjectPickerRenderPipelineStage : public IRenderPipelineStage
	{
	public:

		ObjectPickerRenderPipelineStage()
		{
			m_objectPickerReadback = MakeRef<BFTextureReadback>();
		}

		virtual void OnPostRender() override {}
		virtual void OnRecordPass(const ViewportRenderEvent& event) override
		{
			BF_PROFILE_EVENT()

			GraphBuilder& builder = event.Builder;
			Viewport& viewport = event.Viewport;

			ObjectPickerPassData* params = builder.AllocParameters<ObjectPickerPassData>();

			BFTextureDesc desc;
			desc.Format = DXGI_FORMAT_R32_UINT;
			desc.Width = viewport.GetRenderTarget().Width();
			desc.Height = viewport.GetRenderTarget().Height();
			desc.Flags = BFTextureDesc::RenderTargettable;
			desc.DebugName = "R32 Viewport objectpicker";
			params->RenderTarget = builder.CreateTransientTexture("R32 Viewport objectpicker", desc);

			BFTextureDesc desc2;
			desc2.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			desc2.Width = viewport.GetRenderTarget().Width();
			desc2.Height = viewport.GetRenderTarget().Height();
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
						list.DrawIndexedInstanced(mesh->GPUIndices->NumElements(), 1, 0, 0, 0);
					}

					m_objectPickerReadback->ReadbackCopy(list, data.RenderTarget->Resource());
				});
		}

	private:
		friend class SceneViewport;
		RefPtr<BFTextureReadback> m_objectPickerReadback;
	};



	SceneViewport::SceneViewport()
	{
		m_viewportHandle = Application::Get().GetRenderer().AddViewport();

		Application::Get().GetRenderer().GetViewport(m_viewportHandle).RenderPipeline->RegisterStage<DebugRendererPipelineStage>();
		Application::Get().GetRenderer().GetViewport(m_viewportHandle).RenderPipeline->RegisterStage<ObjectPickerRenderPipelineStage>();


		m_viewportResizeReceiver.Subscribe(Application::Get().GetRenderer().GetViewport(m_viewportHandle).Events.OnResize, BF_BIND_FUNC_PARAM(&SceneViewport::OnResize));
		m_viewportPrerenderReceiver.Subscribe(Application::Get().GetRenderer().GetViewport(m_viewportHandle).Events.OnPreRender, BF_BIND_FUNC_PARAM(&SceneViewport::OnPrerender));
	}
	
	SceneViewport::~SceneViewport()
	{
		Application::Get().GetRenderer().RemoveViewport(m_viewportHandle);
	}
		
	void SceneViewport::OnTick()
	{
		BF_PROFILE_EVENT()

		m_spectatorCam.Tick(Application::Get().GetInput(), Application::Get().GetTime().DeltaTime());

		glm::vec3 position = m_spectatorCam.GetCamera()->Position();
		position.x = std::round(position.x);
		position.y = std::round(position.y);
		position.z = std::round(position.z);

		const uint32_t gridLineCount = 64;
		const glm::vec4 color(0.3f, 0.3f, 0.3f, 0.1f);

		for (int i = position.x - gridLineCount; i <= position.x + gridLineCount; ++i)
		{
			EditorApplication::Get().GetDebugRenderer().DrawLine(glm::vec3(i, 0, position.z - gridLineCount), glm::vec3(i, 0, position.z + gridLineCount), color);
		}

		for (int i = position.z - gridLineCount; i <= position.z + gridLineCount; ++i)
		{
			EditorApplication::Get().GetDebugRenderer().DrawLine(glm::vec3(position.x - gridLineCount, 0, i), glm::vec3(position.x + gridLineCount, 0, i), color);
		}
	}

	void SceneViewport::OnResize(const ViewportResizeEvent& event)
	{
		auto p = m_spectatorCam.GetCamera()->Projection();
		p.AspectRatio = static_cast<float>(event.Size.x) / static_cast<float>(event.Size.y);
		m_spectatorCam.GetCamera()->SetProjection(p);
	}

	void SceneViewport::OnPrerender(const ViewportPrerenderEvent& event)
	{
		BF_PROFILE_EVENT()

		CameraData cameraData;
		cameraData.ViewProjection = m_spectatorCam.GetCamera()->ViewProjectionMatrix();
		cameraData.CameraPosition = m_spectatorCam.GetCamera()->Position();
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
		BF_PROFILE_EVENT()

		ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_NoWindowMenuButton;
		ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), dockspaceFlags);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground);
		const ImVec2 startCursorPos = ImGui::GetCursorPos();

		// Delete selected entity.
		if (ImGui::IsKeyPressed(ImGuiKey_Delete) && ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
		{
			if (EditorApplication::Get().GetEditorViewport().m_selectedEntity)
			{
				EditorApplication::Get().GetEditorViewport().m_selectedEntity.Destroy();
			}
			EditorApplication::Get().GetEditorViewport().m_selectedEntity = Entity();
		}

		// Viewport rendering.
		if (m_viewportHandle.Valid())
		{
			Application::Get().GetRenderer().ImGUIImage(m_viewportHandle);
		}

		if (ImGui::IsKeyPressed(ImGuiKey_W) && !ImGui::IsMouseDown(ImGuiMouseButton_Right))
		{
			m_currentOperation = ImGuizmo::TRANSLATE;
		}
		if (ImGui::IsKeyPressed(ImGuiKey_E) && !ImGui::IsMouseDown(ImGuiMouseButton_Right))
		{
			m_currentOperation = ImGuizmo::ROTATE;
		}
		if (ImGui::IsKeyPressed(ImGuiKey_R) && !ImGui::IsMouseDown(ImGuiMouseButton_Right))
		{
			m_currentOperation = ImGuizmo::SCALE;
		}


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
				m_currentOperation,
				m_objectMovementSpace == ObjectMovementSpace::World ? ImGuizmo::WORLD : ImGuizmo::LOCAL,
				&changableMatrix[0][0]
			);

			if (ImGuizmo::IsUsing())
			{
				entityTransform.SetWorldMatrix(changableMatrix);
			}
		}


		// Toolbar.
		ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4, 0.4, 0.5, 1));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5, 0.5, 0.6, 1));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2, 0.2, 0.4, 1));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

		ImGui::SetCursorPos({startCursorPos.x + 5, startCursorPos.y + 5});
		const ImVec2 toolbarMin = {ImGui::GetCursorScreenPos().x - 3, ImGui::GetCursorScreenPos().y - 3};
		const ImVec2 toolbarMax = { toolbarMin.x + 20 + 10 + 20 + 1 + 20 + 1 + 20 + 6, toolbarMin.y + 20 + 6 };
		const bool hoveringToolbar = ImGui::IsMouseHoveringRect(toolbarMin, toolbarMax);
		ImGui::GetWindowDrawList()->AddRectFilled(toolbarMin, toolbarMax, IM_COL32(0, 0, 0, hoveringToolbar ? 100 : 50), 4.0f);

		{
			const ImVec2 buttonSize = ImVec2(20, 20);
			const std::string buttonText = m_objectMovementSpace == ObjectMovementSpace::World ? FontAwesome::Globe : FontAwesome::Cube;
			// Tool overlay.
			if (ImGui::Button(buttonText.c_str(), buttonSize))
			{
				if (m_objectMovementSpace == ObjectMovementSpace::Local)
					m_objectMovementSpace = ObjectMovementSpace::World;
				else
					m_objectMovementSpace = ObjectMovementSpace::Local;
			}
		}
		ImGui::SameLine(0.0f, 10);
		{
			const ImVec2 buttonSize = ImVec2(20, 20);

			
			const auto pressedColor = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
			const auto normalColor = ImGui::GetStyleColorVec4(ImGuiCol_Button);

			ImGui::PushStyleColor(ImGuiCol_Button, m_currentOperation == ImGuizmo::TRANSLATE ? pressedColor : normalColor);

			if (ImGui::Button(FontAwesome::LeftRight, buttonSize))
			{
				m_currentOperation = ImGuizmo::TRANSLATE;
			}
			ImGui::PopStyleColor();

			ImGui::SameLine(0.0f, 1.0f);

			ImGui::PushStyleColor(ImGuiCol_Button, m_currentOperation == ImGuizmo::ROTATE ? pressedColor : normalColor);
			if (ImGui::Button(FontAwesome::Rotate, buttonSize))
			{
				m_currentOperation = ImGuizmo::ROTATE;
			}
			ImGui::PopStyleColor();

			ImGui::SameLine(0.0f, 1.0f);

			ImGui::PushStyleColor(ImGuiCol_Button, m_currentOperation == ImGuizmo::SCALE ? pressedColor : normalColor);
			if (ImGui::Button(FontAwesome::Expand, buttonSize))
			{
				m_currentOperation = ImGuizmo::SCALE;
			}
			ImGui::PopStyleColor();
		}
		ImGui::PopFont();
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar();

		// Draw icons.
		const glm::ivec2 viewportOffset = glm::ivec2(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y) + glm::ivec2(ImGui::GetWindowContentRegionMin().x, ImGui::GetWindowContentRegionMin().y);
		float iconSize = 24.0f;
		ImFont* font = ImGui::GetIO().Fonts->Fonts[2];

		ImGui::PushFont(font, iconSize);

		for (const auto& [entity, camera, transform] : Application::Get().GetScene().GetEntityRegistry().view<CameraComponent, TransformComponent>().each())
		{
			glm::ivec2 screenPos = WorldToViewport(transform.GetPosition(), m_spectatorCam.GetCamera()->ViewProjectionMatrix(), Application::Get().GetRenderer().GetViewport(m_viewportHandle).Size());
			screenPos += viewportOffset;

			const char* icon = FontAwesome::VideoCamera;
			ImVec2 textSize = ImGui::CalcTextSize(icon);

			const ImVec2 iconPos = ImVec2(screenPos.x - textSize.x * 0.5f, screenPos.y - textSize.y * 0.5f);

			ImGui::GetWindowDrawList()->AddText(iconPos, IM_COL32(255, 255, 255, 180), icon);
		}

		for (const auto& [entity, light, transform] : Application::Get().GetScene().GetEntityRegistry().view<LightComponent, TransformComponent>().each())
		{
			glm::ivec2 screenPos = WorldToViewport(transform.GetPosition(), m_spectatorCam.GetCamera()->ViewProjectionMatrix(), Application::Get().GetRenderer().GetViewport(m_viewportHandle).Size());
			screenPos += viewportOffset;

			const char* icon = FontAwesome::Lightbulb;
			ImVec2 textSize = ImGui::CalcTextSize(icon);

			const ImVec2 iconPos = ImVec2(screenPos.x - textSize.x * 0.5f, screenPos.y - textSize.y * 0.5f);

			ImGui::GetWindowDrawList()->AddText(iconPos, IM_COL32(255, 255, 255, 180), icon);

			EditorApplication::Get().GetDebugRenderer().DrawLine(transform.GetPosition(), transform.GetPosition() + transform.GetForward() * 2.0f, glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));	
		}

		ImGui::PopFont();

		// Draw selected entity primtives.
		if (EditorApplication::Get().GetEditorViewport().m_selectedEntity)
		{
			if (CameraComponent* camera = EditorApplication::Get().GetEditorViewport().m_selectedEntity.TryGetComponent<CameraComponent>())
			{
				TransformComponent& transform = EditorApplication::Get().GetEditorViewport().m_selectedEntity.GetComponent<TransformComponent>();
				EditorApplication::Get().GetDebugRenderer().DrawFrustum(camera->GetViewprojectionMatrrix(transform.GetPosition(), transform.GetRotation()), glm::vec4(1.0f, 1.0f, 0.0f, 0.5f));
			}
		}


		// Object selection.
		const glm::ivec2 mousePos = glm::ivec2(ImGui::GetMousePos().x, ImGui::GetMousePos().y);
		const glm::ivec2 contentPos = glm::ivec2(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y) + glm::ivec2(ImGui::GetWindowContentRegionMin().x, ImGui::GetWindowContentRegionMin().y);
		const glm::ivec2 relativePos = mousePos - contentPos;
		uint32_t readbackID = 0;

		ObjectPickerRenderPipelineStage* objectPickerStage = Application::Get().GetRenderer().GetViewport(m_viewportHandle).RenderPipeline->TryGetStage<ObjectPickerRenderPipelineStage>();

		if (objectPickerStage &&
			objectPickerStage->m_objectPickerReadback->ReadPixel(relativePos, readbackID) &&
			ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
			!ImGuizmo::IsOver() &&
			!hoveringToolbar)
		{
			if (readbackID > 0)
			{
				// We do -1 because the rendred readbackID increments the entity count for entity 0.
				// Thus entity 0 is entity 1
				const uint32_t sceneEntityID = readbackID - 1;

				EditorApplication::Get().GetEditorViewport().m_selectedEntity = static_cast<entt::entity>(sceneEntityID);
				BF_LOG_INFO("Selected entity: %u", sceneEntityID);
			}
			else
			{
				EditorApplication::Get().GetEditorViewport().m_selectedEntity = Entity();
			}
		}

		ImGui::PopStyleVar(2);
		ImGui::End();
	}
}