#pragma once
#include "Butterfly.hpp"
#include "imgui/imgui.h"
#include "ImGui/ImGUIHelpers.hpp"
#include "EditorApplication.hpp"
#include "Core/ThumbnailProcessor.hpp"
#include "Tools/SpectatorCamera.hpp"

namespace Butterfly
{


	class EditorViewport
	{
	public:
		Entity model;
		float m_modelMovementTime = 0;
		SpectatorCamera m_spectatorCam;
		ViewportHandle m_viewportHandle;

		bool m_renderFullscreenViewport = false;

		EditorViewport()
		{
			Application::Get().GetRenderer().GetImGUIRenderEvent().Subscribe(BF_BIND_FUNC(&EditorViewport::OnRenderImGUI));

			Application::Get().GetBlackboard().Register<Camera>(m_spectatorCam.GetCamera(), "ViewCamera");

			m_viewportHandle = Application::Get().GetRenderer().AddViewport();
			Application::Get().GetRenderer().GetViewportEvents(m_viewportHandle).OnResize.Subscribe([&](const ViewportResizeEvent& ev)
				{
					auto p = m_spectatorCam.GetCamera()->Projection();
					p.AspectRatio = static_cast<float>(ev.Size.x) / static_cast<float>(ev.Size.y);
					m_spectatorCam.GetCamera()->SetProjection(p);
				});
		}

		void Tick()
		{
			m_spectatorCam.Tick(Application::Get().GetInput(), Application::Get().GetTime().DeltaTime());

			if (Application::Get().GetInput().IsKeyDown(BFB_F11))
			{
				Application::Get().GetWindow().SetFullscreen(!Application::Get().GetWindow().Fullscreen());
			}

			if (Application::Get().GetInput().IsKeyDown(BFB_V))
			{
				m_viewportHandle = Application::Get().GetRenderer().AddViewport();
			}

			if (Application::Get().GetInput().IsKeyDown(BFB_B))
			{
				Application::Get().GetRenderer().RemoveViewport(m_viewportHandle);
			}

			if (Application::Get().GetInput().IsKeyDown(BFB_T))
			{
				model = Application::Get().GetScene().CreateEntity();

				model.AddComponent<TransformComponent>();
				model.AddComponent<MeshRendererComponent>();
			}

			if (Application::Get().GetInput().IsKeyDown(BFB_F))
			{
				m_renderFullscreenViewport = !m_renderFullscreenViewport;
			}

			if (Application::Get().GetInput().IsKeyPressed(BFB_R))
			{
				if (model)
				{
					m_modelMovementTime += Application::Get().GetTime().DeltaTime();

					TransformComponent& tr = model.GetComponent<TransformComponent>();

					glm::vec3 position = tr.GetPosition();
					position.y = glm::sin(m_modelMovementTime * 3);
					tr.SetPosition(position);

					tr.SetRotation(glm::quat(glm::vec3(0.0f, m_modelMovementTime * 5, 0.0f)));
				}
			}
		}

		void OnRenderImGUI()
		{
			if (m_renderFullscreenViewport)
			{
				ImGuiViewport* viewport = ImGui::GetMainViewport();

				ImGui::SetNextWindowPos(viewport->WorkPos);
				ImGui::SetNextWindowSize(viewport->WorkSize);
				ImGui::SetNextWindowViewport(viewport->ID);

				ImGuiWindowFlags windowFlags =
					ImGuiWindowFlags_NoDecoration |
					ImGuiWindowFlags_NoMove |
					ImGuiWindowFlags_NoSavedSettings |
					ImGuiWindowFlags_NoBringToFrontOnFocus |
					ImGuiWindowFlags_NoFocusOnAppearing;

				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
				ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

				ImGui::Begin("Fullscreen Viewport", nullptr, windowFlags);

				if (m_viewportHandle.Valid())
				{
					Application::Get().GetRenderer().ImGUIImage(m_viewportHandle);
				}

				ImGui::End();

				ImGui::PopStyleVar(2);

				return; 
			}

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

				if (m_viewportHandle.Valid())
				{
					Application::Get().GetRenderer().ImGUIImage(m_viewportHandle);
				}

				ImGui::PopStyleVar(2);
				ImGui::End();

				ImGui::Begin("Properties");

				if (model)
				{
					TransformComponent& tr = model.GetComponent<TransformComponent>();

					if (ImGui::CollapsingHeader("TransformComponent", ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::PushID("TransformComponent");

						glm::vec3 position = tr.GetPosition();
						if (ImGUIHelpers::DrawVec3Control("Position", position))
						{
							tr.SetPosition(position);
						}


						glm::vec3 eulerRotation = glm::degrees(glm::eulerAngles(tr.GetRotation()));
						if (ImGUIHelpers::DrawVec3Control("Rotation", eulerRotation))
						{
							tr.SetRotation(glm::quat(glm::radians(eulerRotation)));
						}


						glm::vec3 scale = tr.GetScale();
						if (ImGUIHelpers::DrawVec3Control("Scale", scale, 1.0f))
						{
							tr.SetScale(scale);
						}

						ImGui::PopID();
					}


					MeshRendererComponent& mr = model.GetComponent<MeshRendererComponent>();

					if (ImGui::CollapsingHeader("MeshRenderer", ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::PushID("MeshRenderer");

						ImGui::Button("Drop mesh here", ImVec2(200.0f, 40.0f));
						if (ImGui::BeginDragDropTarget())
						{
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET"))
							{
								Application::Get().GetAssetManager().Acquire<MeshAsset>(*(UUID*)payload->Data, mr.MeshHandle);
							}

							ImGui::EndDragDropTarget();
						}

						AssetMetadata meta;
						if (Application::Get().GetAssetManager().GetAssetRegistry().Find(mr.MeshHandle.GetID(), meta))
						{
							ImGui::Text("%s", std::filesystem::path(meta.Path).stem().string().c_str());
						}
						else
						{
							ImGui::Text("%s", "No Reference");
						}

						ImGui::PopID();
					}
				}

				ImGui::End();
			}


			{
				ImGui::Begin("Asset View");

				ImGui::TextDisabled("%d assets", (int)Application::Get().GetAssetManager().GetAssetRegistry().GetAll().size());
				ImGui::Separator();
				ImGui::Spacing();

				const float cellSize = 96.0f;
				const float cellPadding = 8.0f;
				const float iconSize = 64.0f;

				float panelWidth = ImGui::GetContentRegionAvail().x;
				int columnCount = (int)(panelWidth / (cellSize + cellPadding));
				if (columnCount < 1) columnCount = 1;

				ImGui::BeginChild("AssetGrid", ImVec2(0, 0), true);

				if (ImGui::BeginTable("AssetGridTable", columnCount, ImGuiTableFlags_SizingFixedFit))
				{
					int i = 0;
					for (auto& [id, meta] : Application::Get().GetAssetManager().GetAssetRegistry().GetAll())
					{
						ImGui::TableNextColumn();
						ImGui::PushID((int)std::hash<Butterfly::UUID>()(id));

						ImVec2 cellStart = ImGui::GetCursorScreenPos();
						float cellHeight = iconSize + 32.0f;

						ImGui::InvisibleButton("##cell", ImVec2(cellSize, cellHeight));
						const bool hovered = ImGui::IsItemHovered();
						const bool doubleClicked = hovered && ImGui::IsMouseDoubleClicked(0);

						ImDrawList* dl = ImGui::GetWindowDrawList();
						ImU32 iconColor = ImColor(10, 20, 50);
						float iconOffsetX = (cellSize - iconSize) * 0.5f;

						if (ImGui::BeginDragDropSource())
						{
							UUID id = meta.ID;

							ImGui::SetDragDropPayload(
								"ASSET",
								&id,
								sizeof(UUID)
							);

							ImGui::Text("Dragging %s", std::filesystem::path(meta.Path).filename().string().c_str());

							ImGui::EndDragDropSource();
						}

						if (hovered)
						{
							dl->AddRect(
								ImVec2(cellStart.x, cellStart.y),
								ImVec2(cellStart.x + cellSize, cellStart.y + cellHeight),
								IM_COL32(255, 255, 255, 40), 4.0f
							);
						}

						if (doubleClicked)
						{
							AssetHandle<MeshAsset> objMesh;
							Application::Get().GetAssetManager().Acquire<MeshAsset>(meta.ID, objMesh);
							MeshAsset* asset = Application::Get().GetAssetManager().Resolve(objMesh);

							model = Application::Get().GetScene().CreateEntity();

							model.AddComponent<TransformComponent>();
							model.AddComponent<MeshRendererComponent>();
							model.GetComponent<MeshRendererComponent>().MeshHandle = objMesh;
						}


						if (ThumbnailProcessor::IsSupportedImageType(meta.Path) && !EditorApplication::Get().GetEditorCache().Exists(meta.ID))
						{
							RefPtr<Thumbnail> thumbnail = ThumbnailProcessor::GetThumbnailFromFile(meta.Path);
							thumbnail = ThumbnailProcessor::Resize(*thumbnail, 64, 64);
							ThumbnailCacheEntry entry(thumbnail);
							EditorApplication::Get().GetEditorCache().Add<ThumbnailCacheEntry>(meta.ID, entry);
							BF_CORE_LOG_INFO("Awooof");
						}


						ThumbnailCacheEntry cacheEntry;
						if (EditorApplication::Get().GetEditorCache().Get<ThumbnailCacheEntry>(meta.ID, cacheEntry))
						{
							RefPtr<Thumbnail> thumbnail = cacheEntry.GetThumbnail();
							ImGui::SetCursorScreenPos({ cellStart.x + iconOffsetX, cellStart.y });

							ImGui::Image(thumbnail->GetImGUITextureID(), { iconSize, iconSize });
						}
						else
						{
							dl->AddRectFilled(
								ImVec2(cellStart.x + iconOffsetX, cellStart.y),
								ImVec2(cellStart.x + iconOffsetX + iconSize, cellStart.y + iconSize),
								iconColor, 4.0f);
						}

						std::string name = std::filesystem::path(meta.Path).filename().string();
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

	private:
		ImVec2 m_viewportSize;
		ImVec2 m_viewportPos;
	};
}