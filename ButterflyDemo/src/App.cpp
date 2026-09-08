#include "App.hpp"
#include "glm/gtx/quaternion.hpp"
#include <numeric>
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace Butterfly
{
	static bool DrawVec3Control(const char* label, glm::vec3& values, float resetValue = 0.0f, float speed = 0.1f, float min = 0.0f, float max = 0.0f)
	{
		bool changed = false;
		ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts.Size > 1 ? io.Fonts->Fonts[1] : io.Fonts->Fonts[0];

		ImGui::PushID(label);

		float fullWidth = ImGui::GetContentRegionAvail().x;
		float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight, lineHeight };

		float groupSpacing = ImGui::GetStyle().ItemSpacing.x;
		float dragWidth = (fullWidth - 3.0f * buttonSize.x - 2.0f * groupSpacing) / 3.0f;

		auto axisControl = [&](const char* axisLabel, float& v, bool isLast,
			ImVec4 baseColor) {
				ImGui::PushStyleColor(ImGuiCol_Button, baseColor);
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
				ImGui::PushFont(boldFont);
				ImGui::Button(axisLabel, buttonSize);
				ImGui::PopFont();
				ImGui::PopStyleColor(2);

				ImGui::SameLine(0.0f, 0.0f);

				ImGui::PushItemWidth(dragWidth);
				std::string dragId = std::string("##") + axisLabel;
				char fmt[] = "%.2f";
				if (min == 0.0f && max == 0.0f)
				{
					changed |= ImGui::DragFloat(dragId.c_str(), &v, speed, 0.0f, 0.0f, fmt);
				}
				else
				{
					changed |= ImGui::DragFloat(dragId.c_str(), &v, speed, min, max, fmt);
				}
				ImGui::PopItemWidth();

				if (!isLast)
				{
					ImGui::SameLine(0.0f, groupSpacing);
				}
			};

		axisControl("X", values.x, false, ImVec4(0.72f, 0.16f, 0.16f, 1.0f));
		axisControl("Y", values.y, false, ImVec4(0.16f, 0.55f, 0.16f, 1.0f));
		axisControl("Z", values.z, true, ImVec4(0.14f, 0.28f, 0.72f, 1.0f));

		ImGui::PopID();

		return changed;
	}

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

		Application::Get().GetRenderer().OnImGUIRender.Subscribe([&](FrameData& data)
			{
				ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_NoWindowMenuButton;
				ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), dockspaceFlags);

				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
				ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
				ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground);

				Application::Get().GetRenderer().RenderImGUIImage(data, 0);

				ImGui::PopStyleVar(2);
				ImGui::End();

				ImGui::Begin("Properties");

				Transform& tr = model.GetComponent<Transform>();

				if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::PushID("Transform");

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

				MeshRenderer& mr = model.GetComponent<MeshRenderer>();

				if (ImGui::CollapsingHeader("MeshRenderer", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::PushID("MeshRenderer");
					ImGui::Text("Nothing to see here");	
					ImGui::PopID();
				}

				ImGui::End();




				ImGui::Begin("Assets");
				ImGui::End();


				entt::registry& registry = Application::Get().GetScene().GetEntityRegistry();

				ImGui::Begin("Scene Hierarchy");

				auto view = registry.view<Transform>();
				int entityCount = 0;
				for (auto e : view) (void)e, entityCount++;

				ImGui::TextDisabled("%d entities", entityCount);

				ImGui::Separator();
				ImGui::Spacing();

				// --- Entity list ---
				ImGui::BeginChild("EntityList", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollWithMouse);

				int rowIndex = 0;
				for (auto entity : view)
				{
					ImGui::PushID((int)entity);

					// Alternating row background for readability
					ImVec2 rowMin = ImGui::GetCursorScreenPos();
					float rowHeight = ImGui::GetFrameHeight();
					if (rowIndex % 2 == 1) {
						ImDrawList* dl = ImGui::GetWindowDrawList();
						ImVec2 rowMax(rowMin.x + ImGui::GetContentRegionAvail().x, rowMin.y + rowHeight);
						dl->AddRectFilled(rowMin, rowMax, IM_COL32(255, 255, 255, 6));
					}

					// Small colored dot as a lightweight "icon"
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
			});

		model = Application::Get().GetScene().CreateEntity();

		model.AddComponent<Transform>();
		model.AddComponent<MeshRenderer>();
		model.GetComponent<MeshRenderer>().LoadTestModel();
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

			Transform& tr = model.GetComponent<Transform>();

			glm::vec3 position = tr.GetPosition();
			position.y = glm::sin(m_modelMovementTime * 3);
			tr.SetPosition(position);

			tr.SetRotation(glm::quat(glm::vec3(0.0f, m_modelMovementTime * 5, 0.0f)));
		}

		m_input.Poll();
	}

	void SandboxLayer::OnShutdown()
	{
		BF_PROFILE_EVENT()

		FullscreenQuad::ShutDown();
	}
}
