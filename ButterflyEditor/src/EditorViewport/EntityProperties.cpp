#include "EditorViewport/EntityProperties.hpp"
#include "EditorViewport/EditorViewport.hpp"
#include "Core/EditorApplication.hpp"
#include "ImGui/ImGuiHelpers.hpp"
#include "ImGui/FontAwesomeIcons.hpp"

namespace Butterfly
{
	EntityProperties::EntityProperties()
	{
	}

	EntityProperties::~EntityProperties()
	{
	}

	void EntityProperties::OnTick()
	{
	}

	void EntityProperties::OnRenderImGUI()
	{
		ImGui::Begin("Properties");

		Entity& selectedEntity = EditorApplication::Get().GetEditorViewport().m_selectedEntity;

		if (selectedEntity)
		{
			TransformComponent& tr = selectedEntity.GetComponent<TransformComponent>();

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


			MeshRendererComponent* mr = selectedEntity.TryGetComponent<MeshRendererComponent>();

			if (mr && ImGui::CollapsingHeader("MeshRenderer", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::PushID("MeshReference");

				if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsItemHovered())
				{
					ImGui::OpenPopup("MeshRendererContextMenu");
				}

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

				if (ImGui::BeginPopup("MeshRendererContextMenu"))
				{
					if (ImGui::MenuItem("Remove MeshRendererComponent"))
					{
						selectedEntity.RemoveComponent<MeshRendererComponent>();
						ImGui::CloseCurrentPopup();
					}
					ImGui::EndPopup();
				}

				ImGui::PopID();

			}

			SkyboxComponent* sb = selectedEntity.TryGetComponent<SkyboxComponent>();
			if (sb)
			{
				if (ImGui::CollapsingHeader("SkyboxComponent", ImGuiTreeNodeFlags_DefaultOpen))
				{
					if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsItemHovered())
					{
						ImGui::OpenPopup("SkyboxComponentContextMenu");
					}

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

				if (ImGui::BeginPopup("SkyboxComponentContextMenu"))
				{
					if (ImGui::MenuItem("Remove SkyboxComponent"))
					{
						selectedEntity.RemoveComponent<SkyboxComponent>();
						ImGui::CloseCurrentPopup();
					}
					ImGui::EndPopup();
				}

				ImGui::PopID();
			}

			ImGui::Dummy(ImVec2(0.0f, 10.0f));
			const ImVec2 size = ImGui::GetContentRegionAvail();
			if (ImGui::Button("Add Component", ImVec2(size.x, 20.0f)))
			{
				ImGui::OpenPopup("AddComponentPopup");
			}

			if (ImGui::BeginPopup("AddComponentPopup"))
			{
				ComponentRegistry::RunOnAllComponents([&]<typename T>()
				{
					if (!selectedEntity.HasComponent<T>())
					{
						const std::string_view componentName = entt::resolve<T>().name();
						if (ImGui::MenuItem(componentName.data()))
						{
							selectedEntity.AddComponent<T>();
							ImGui::CloseCurrentPopup();
						}
					}
				});

				ImGui::EndPopup();
			}
		}
		ImGui::End();
	}
}