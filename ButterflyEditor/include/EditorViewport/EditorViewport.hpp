#pragma once
#include "Butterfly.hpp"
#include "imgui/imgui.h"
#include "ImGui/ImGUIHelpers.hpp"
#include "Core/EditorApplication.hpp"
#include "Core/ThumbnailProcessor.hpp"
#include "EditorViewport/SceneViewport.hpp"
#include "EditorViewport/SceneHierarchy.hpp"
#include "EditorViewport/EditorViewportExtention.hpp"
#include "ImGui/FontAwesomeIcons.hpp"

namespace Butterfly
{
	class EditorViewport
	{
	public:

		Skybox skybox;

		EditorViewport()
		{
			m_ImGUIRenderReceiver.Subscribe(Application::Get().GetRenderer().GetImGUIRenderEvent(), BF_BIND_FUNC(&EditorViewport::OnRenderImGUI));

			m_viewportExtentions.push_back(MakeRef<SceneViewport>());
			m_viewportExtentions.push_back(MakeRef<SceneHierarchy>());
		}

		void Tick()
		{
			BF_PROFILE_EVENT()

			for (auto& ext : m_viewportExtentions)
			{
				ext->OnTick();
			}

			if (Application::Get().GetInput().IsKeyDown(BFB_F11))
			{
				Application::Get().GetWindow().SetFullscreen(!Application::Get().GetWindow().Fullscreen());
			}


			if (Application::Get().GetInput().IsKeyDown(BFB_T))
			{
				m_selectedEntity = Application::Get().GetScene().CreateEntity();

				m_selectedEntity.AddComponent<MeshRendererComponent>();
				m_selectedEntity.AddComponent<SkyboxComponent>();
			}

			if (Application::Get().GetInput().IsKeyDown(BFB_Y))
			{
				auto node = Application::Get().GetScene().Serialize();
				

			}
		}

		void OnRenderImGUI()
		{
			BF_PROFILE_EVENT()

			for (auto& ext : m_viewportExtentions)
			{
				ext->OnRenderImGUI();
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
				ImGui::Begin("Properties");

				if (m_selectedEntity)
				{
					TransformComponent& tr = m_selectedEntity.GetComponent<TransformComponent>();

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


					MeshRendererComponent* mr = m_selectedEntity.TryGetComponent<MeshRendererComponent>();

					if (mr && ImGui::CollapsingHeader("MeshRenderer", ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::PushID("MeshReference");

						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.20f, 0.20f, 1.0f));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));

						const float fieldHeight = 20.0f;
						const float fieldWidth = 200.0f;
						const float pickerWidth = fieldHeight;
		
						ImGuiIO& io = ImGui::GetIO();

						ImGui::Button(FontAwesome::Search, ImVec2(pickerWidth, fieldHeight));
						ImGui::SameLine(0.0f, 0.0f);

						std::string meshName = "No Reference";

						if (mr->GetMeshHandle())
						{
							AssetMetadata meta;
							Application::Get().GetAssetManager().GetAssetRegistry().Find(mr->GetMeshHandle().GetID(), meta);
							meshName = std::filesystem::path(meta.Path).stem().string();
						}

						ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
						ImGui::Button(meshName.c_str(), ImVec2(fieldWidth, fieldHeight));
						ImGui::PopStyleVar();

						if (ImGui::BeginDragDropTarget())
						{
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET"))
							{
								if (payload->DataSize == sizeof(UUID))
								{
									AssetHandle<MeshAsset> handle;
									Application::Get().GetAssetManager().Acquire<MeshAsset>(*(UUID*)payload->Data, handle);
									mr->SetMeshHandle(handle);
								}
							}

							ImGui::EndDragDropTarget();
						}

						ImGui::PopStyleColor(3);

						ImGui::PopID();

					}

					SkyboxComponent* sb = m_selectedEntity.TryGetComponent<SkyboxComponent>();
					if(sb)
					{
						 if (ImGui::CollapsingHeader("SkyboxComponent", ImGuiTreeNodeFlags_DefaultOpen))
						 {
							 ImGui::PushID("SkyboxComponent");
							 for (int i = 0; i < 6; i++)
							 {
								 std::string faceName;
								 switch (i)
								 {
								 case 0: faceName = "Right"; break;
								 case 1: faceName = "Left"; break;
								 case 2: faceName = "Top"; break;
								 case 3: faceName = "Bottom"; break;
								 case 4: faceName = "Front"; break;
								 case 5: faceName = "Back"; break;
								 }
								 ImGui::Text("%s", faceName.c_str());
								 ImGui::SameLine();
								 ImGui::Button("Drop texture here", ImVec2(200.0f, 20.0f));
								 if (ImGui::BeginDragDropTarget())
								 {
									 if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET"))
									 {
										 AssetHandle<TextureAsset> handle;
										 Application::Get().GetAssetManager().Acquire<TextureAsset>(*(UUID*)payload->Data, handle);
										 sb->SetTextureHandle(i, handle);
									 }
									 ImGui::EndDragDropTarget();
								 }
								 AssetMetadata meta;
								 if (Application::Get().GetAssetManager().GetAssetRegistry().Find(sb->GetTextureHandle(i).GetID(), meta))
								 {
									 ImGui::Text("%s", std::filesystem::path(meta.Path).stem().string().c_str());
								 }
								 else
								 {
									 ImGui::Text("%s", "No Reference");
								 }
							 }
						 }
						ImGui::PopID();
					}
				}

				ImGui::End();
			}

			auto view = Application::Get().GetScene().GetEntityRegistry().view<SkyboxComponent>();
			for (const auto& [entity, sb] : view.each())
			{
				if(sb.IsDirty())
				{
					skybox.LoadSkybox(sb);
					sb.ClearDirty();
				}
			}

			{
				ImGui::Begin("Asset View");

				ImGui::TextDisabled("%d assets", (int)Application::Get().GetAssetManager().GetAssetRegistry().GetAll().size());
				ImGui::Separator();
				ImGui::Spacing();

				const float cellSize = 96.0f;
				const float cellPadding = 8.0f;
				const float iconSize = 64.0f;

				ImGui::BeginChild("AssetGrid", ImVec2(0, 0), true);

				const float panelWidth = ImGui::GetContentRegionAvail().x;
				int columnCount = (int)((panelWidth + cellPadding) / (cellSize + cellPadding));
				if (columnCount < 1) columnCount = 1;

				if (ImGui::BeginTable("AssetGridTable", columnCount, ImGuiTableFlags_SizingFixedSame | ImGuiTableFlags_NoBordersInBody))
				{
					for (int column = 0; column < columnCount; ++column)
						ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed, cellSize);

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


						if (ImGui::BeginPopupContextItem())
						{
							if (ImGui::MenuItem("Show in Explorer"))
							{
								system(("explorer.exe /select," + meta.Path).c_str());
							}

							ImGui::EndPopup();
						}
						 
						if (ImGui::BeginDragDropSource())
						{
							UUID id = meta.ID;

							ImGui::SetDragDropPayload("ASSET", &id, sizeof(UUID));
							ImGui::Text("Dragging %s", std::filesystem::path(meta.Path).filename().string().c_str());

							ImGui::EndDragDropSource();
						}

						if (hovered)
						{
							dl->AddRect(ImVec2(cellStart.x, cellStart.y), ImVec2(cellStart.x + cellSize, cellStart.y + cellHeight), IM_COL32(255, 255, 255, 40), 4.0f);
						}

						if (doubleClicked)
						{
							if (meta.Extention == ".bfscene")
							{
								const std::string& scene = FileSystem::ReadText(meta.Path);
								Application::Get().GetScene().Deserialize(scene, std::filesystem::path(meta.Path).stem().string());

								ImGui::PopID();
								ImGui::EndTable();
								ImGui::EndChild();
								ImGui::End();
								return;
							}
						}

						if (ThumbnailProcessor::IsSupportedImageType(meta.Path) && !EditorApplication::Get().GetEditorCache().Exists(meta.ID))
						{
							RefPtr<Thumbnail> thumbnail = ThumbnailProcessor::GetThumbnailFromFile(meta.Path);
							thumbnail = ThumbnailProcessor::Resize(*thumbnail, 64, 64);

							ThumbnailCacheEntry entry(thumbnail);
							EditorApplication::Get().GetEditorCache().Add<ThumbnailCacheEntry>(meta.ID, entry);
						}

						ThumbnailCacheEntry cacheEntry;

						if (EditorApplication::Get().GetEditorCache().Get<ThumbnailCacheEntry>(meta.ID, cacheEntry))
						{
							RefPtr<Thumbnail> thumbnail = cacheEntry.GetThumbnail();

							ImGui::SetCursorScreenPos(ImVec2(cellStart.x + iconOffsetX, cellStart.y));
							ImGui::Image(thumbnail->GetImGUITextureID(), ImVec2(iconSize, iconSize));
						}
						else
						{
							dl->AddRectFilled(ImVec2(cellStart.x + iconOffsetX, cellStart.y), ImVec2(cellStart.x + iconOffsetX + iconSize, cellStart.y + iconSize), iconColor, 4.0f);
						}

						std::string name = std::filesystem::path(meta.Path).filename().string();

						ImGui::SetCursorScreenPos(ImVec2(cellStart.x, cellStart.y + iconSize + 4.0f));
						ImGui::PushTextWrapPos(cellStart.x + cellSize);
						ImGui::TextWrapped("%s", name.c_str());
						ImGui::PopTextWrapPos();

						ImGui::PopID();
					}

					ImGui::EndTable();
				}

				ImGui::EndChild();
				ImGui::End();
			}
		}

		Entity m_selectedEntity;
	private:

		float m_modelMovementTime = 0;

		std::vector<RefPtr<IEditorViewportExtention>> m_viewportExtentions;
		EventReceiver<> m_ImGUIRenderReceiver;
	};
}