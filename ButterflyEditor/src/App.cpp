#include "App.hpp"
#include "glm/gtx/quaternion.hpp"
#include <numeric>
#include "ImGUI/ImGUIHelpers.hpp"

namespace Butterfly
{
	void SandboxLayer::OnInit()
	{
		BF_PROFILE_EVENT()

		m_input.Init(&Application::Get().GetWindow());

		Application::Get().GetBlackboard().Register<Camera>(m_spectatorCam.GetCamera(), "ViewCamera");

		Application::Get().GetRenderer().OnViewportResize.Subscribe([&](const ViewportResizeEvent& ev)
			{
				auto p = m_spectatorCam.GetCamera()->Projection();
				p.AspectRatio = static_cast<float>(ev.Size.x) / static_cast<float>(ev.Size.y);
				m_spectatorCam.GetCamera()->SetProjection(p);
			});

		Application::Get().GetRenderer().OnImGUIRender.Subscribe(BF_BIND_FUNC(&SandboxLayer::ImGUIRender));

		model = Application::Get().GetScene().CreateEntity();

		model.AddComponent<TransformComponent>();
		model.AddComponent<MeshRendererComponent>();
		model.GetComponent<MeshRendererComponent>().LoadTestModel();
	}

	void SandboxLayer::OnTick()
	{
		BF_PROFILE_FRAME("SandboxLayer::OnTick");
		m_spectatorCam.Tick(m_input, Application::Get().GetTime().DeltaTime());

		if (m_input.IsKeyDown(BFB_F11))
		{
			Application::Get().GetWindow().SetFullscreen(!Application::Get().GetWindow().Fullscreen());
		}

		if (m_input.IsKeyPressed(BFB_R))
		{
			m_modelMovementTime += Application::Get().GetTime().DeltaTime();

			TransformComponent& tr = model.GetComponent<TransformComponent>();

			glm::vec3 position = tr.GetPosition();
			position.y = glm::sin(m_modelMovementTime * 3);
			tr.SetPosition(position);

			tr.SetRotation(glm::quat(glm::vec3(0.0f, m_modelMovementTime * 5, 0.0f)));
		}

		m_input.Poll();
	}

	void SandboxLayer::ImGUIRender(FrameData& data)
	{
		{
			if (ImGui::BeginMainMenuBar())
			{
				if (ImGui::BeginMenu("File"))
				{
					if (ImGui::MenuItem("Import"))
					{
						std::string path;
						if (Application::Get().GetWindow().OpenFileDialog(path))
						{
							AssetMetadata meta;
							if (Application::Get().GetAssetManager().GetAssetRegistry().ImportFromDisk(path, meta))
							{
								BF_LOG_INFO("Yippie");
							}
						}
					}

					if (ImGui::MenuItem("Open"))
					{
					}

					if (ImGui::MenuItem("Save"))
					{
					}

					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Tools"))
				{
					if (ImGui::MenuItem("Settings"))
					{
					}

					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Help"))
				{
					ImGui::MenuItem("About");

					ImGui::EndMenu();
				}

				ImGui::EndMainMenuBar();
			}
		}

		{
			ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_NoWindowMenuButton;
			ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), dockspaceFlags);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground);

			Application::Get().GetRenderer().ImGUIImage(data, 0);

			ImGui::PopStyleVar(2);
			ImGui::End();

			ImGui::Begin("Properties");

			TransformComponent& tr = model.GetComponent<TransformComponent>();

			if (ImGui::CollapsingHeader("TransformComponent", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::PushID("TransformComponent");

				glm::vec3 position = tr.GetPosition();
				if (DrawVec3Control("Position", position, 0.0f, 0.1f))
				{
					tr.SetPosition(position);
				}


				glm::vec3 eulerRotation = glm::degrees(glm::eulerAngles(tr.GetRotation()));
				if (DrawVec3Control("Rotation", eulerRotation, 0.0f, 0.5f))
				{
					tr.SetRotation(glm::quat(glm::radians(eulerRotation)));
				}


				glm::vec3 scale = tr.GetScale();
				if (DrawVec3Control("Scale", scale, 1.0f, 0.05f, 0.0001f, 0.0f))
				{
					tr.SetScale(scale);
				}

				ImGui::PopID();
			}

			MeshRendererComponent& mr = model.GetComponent<MeshRendererComponent>();

			if (ImGui::CollapsingHeader("MeshRenderer", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::PushID("MeshRenderer");
				ImGui::Text("Nothing to see here");
				ImGui::PopID();
			}

			ImGui::End();
		}


		{
			ImGui::Begin("Asset View");

			ImGui::TextDisabled("%d assets", (int)Application::Get().GetAssetManager().GetAssetRegistry().GetAll().size());
			ImGui::Separator();
			ImGui::Spacing();

			// --- Grid sizing, Explorer-style: fixed cell size, columns computed from available width ---
			const float cellSize = 96.0f;       // icon + label footprint per item
			const float cellPadding = 8.0f;
			const float iconSize = 64.0f;

			float panelWidth = ImGui::GetContentRegionAvail().x;
			int columnCount = (int)(panelWidth / (cellSize + cellPadding));
			if (columnCount < 1) columnCount = 1;

			ImGui::BeginChild("AssetGrid", ImVec2(0, 0), true);

			if (ImGui::BeginTable("AssetGridTable", columnCount, ImGuiTableFlags_SizingFixedFit)) {
				int i = 0;
				for (auto& [id, meta] : Application::Get().GetAssetManager().GetAssetRegistry().GetAll())
				{
					ImGui::TableNextColumn();
					ImGui::PushID((int)std::hash<Butterfly::UUID>()(id));

					ImVec2 cellStart = ImGui::GetCursorScreenPos();

					ImDrawList* dl = ImGui::GetWindowDrawList();
					ImU32 iconColor = ImColor(10, 20, 50);
					float iconOffsetX = (cellSize - iconSize) * 0.5f;

					dl->AddRectFilled(ImVec2(cellStart.x + iconOffsetX, cellStart.y), ImVec2(cellStart.x + iconOffsetX + iconSize, cellStart.y + iconSize), iconColor, 4.0f);


					bool hovered = ImGui::IsItemHovered();

					if (hovered) {
						dl->AddRect(
							ImVec2(cellStart.x, cellStart.y),
							ImVec2(cellStart.x + cellSize, cellStart.y + iconSize + 32.0f),
							IM_COL32(255, 255, 255, 40), 4.0f
						);
					}

					std::string name = std::filesystem::path(meta.Path).stem().string();
					float textWidth = ImGui::CalcTextSize(name.c_str()).x;
					float textOffsetX = (cellSize - std::min(textWidth, cellSize)) * 0.5f;

					ImGui::SetCursorScreenPos(ImVec2(cellStart.x + std::max(textOffsetX, 0.0f), cellStart.y + iconSize + 4.0f));
					ImGui::PushTextWrapPos(cellStart.x + cellSize);
					ImGui::TextWrapped("%s", name.c_str());
					ImGui::PopTextWrapPos();

					ImGui::PopID();
					i++;
				}
				ImGui::EndTable();
			}

			ImGui::EndChild();
			ImGui::End();
		}

		{
			entt::registry& registry = Application::Get().GetScene().GetEntityRegistry();

			ImGui::Begin("Scene Hierarchy");

			auto view = registry.view<TransformComponent>();
			int entityCount = 0;
			for (auto e : view) (void)e, entityCount++;

			ImGui::TextDisabled("%d entities", entityCount);

			ImGui::Separator();
			ImGui::Spacing();


			ImGui::BeginChild("EntityList", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollWithMouse);

			int rowIndex = 0;
			for (auto entity : view)
			{
				ImGui::PushID((int)entity);

				ImVec2 rowMin = ImGui::GetCursorScreenPos();
				float rowHeight = ImGui::GetFrameHeight();
				if (rowIndex % 2 == 1) {
					ImDrawList* dl = ImGui::GetWindowDrawList();
					ImVec2 rowMax(rowMin.x + ImGui::GetContentRegionAvail().x, rowMin.y + rowHeight);
					dl->AddRectFilled(rowMin, rowMax, IM_COL32(255, 255, 255, 6));
				}

				ImVec2 cursor = ImGui::GetCursorScreenPos();
				ImDrawList* dl = ImGui::GetWindowDrawList();
				ImVec2 dotCenter(cursor.x + 8.0f, cursor.y + rowHeight * 0.5f);
				dl->AddCircleFilled(dotCenter, 4.0f, IM_COL32(80, 200, 180, 255));
				ImGui::Dummy(ImVec2(18.0f, 0.0f));
				ImGui::SameLine();
				NameComponent& name = registry.get<NameComponent>(entity);
				ImGui::Text("%s", name.Name.c_str());

				ImGui::PopID();
				rowIndex++;
			}

			ImGui::EndChild();
			ImGui::End();
		}
	}

	void SandboxLayer::OnShutdown()
	{
		BF_PROFILE_EVENT()

		FullscreenQuad::ShutDown();
	}
}
