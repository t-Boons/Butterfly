#include "EditorViewport/SceneHierarchy.hpp"
#include "EditorViewport/EditorViewport.hpp"


namespace Butterfly
{
	SceneHierarchy::SceneHierarchy()
	{

	}
	SceneHierarchy::~SceneHierarchy()
	{

	}

	void SceneHierarchy::OnTick()
	{

	}

	void SceneHierarchy::OnRenderImGUI()
	{
		ImGui::Begin("Scene Hierarchy");

		if (ImGui::IsKeyPressed(ImGuiKey_Delete) && ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
		{
			if (EditorApplication::Get().GetEditorViewport().m_selectedEntity)
			{
				EditorApplication::Get().GetEditorViewport().m_selectedEntity.Destroy();
			}
			EditorApplication::Get().GetEditorViewport().m_selectedEntity = Entity();
		}

		if (ImGui::BeginPopupContextWindow("SceneHierarchyContext"))
		{
			if (ImGui::MenuItem("New Game Object"))
			{
				EditorApplication::Get().GetEditorViewport().m_selectedEntity = Application::Get().GetScene().CreateEntity();
			}

			ImGui::EndPopup();
		}

		// We do -1 because the root entity is not counted as a game object
		const uint32_t numEntities = static_cast<uint32_t>(Application::Get().GetScene().GetEntityRegistry().view<TransformComponent>().size() - 1);

		ImGui::TextDisabled("%d entities", numEntities);
		ImGui::Separator();
		ImGui::Spacing();

		const Entity& root = Application::Get().GetScene().GetRootEntity();

		ImGui::BeginChild("EntityList", ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysHorizontalScrollbar);

		if (ImGui::BeginPopupContextWindow("EntityListContext"))
		{
			if (ImGui::MenuItem("New Game Object"))
			{
				EditorApplication::Get().GetEditorViewport().m_selectedEntity = Application::Get().GetScene().CreateEntity();
			}

			ImGui::EndPopup();
		}

		ImGui::PushStyleColor(ImGuiCol_Header, IM_COL32(100, 100, 200, 255));
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(30, 30, 30, 225));
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, IM_COL32(30, 30, 30, 225));

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

		DrawHierarchy(root.GetComponent<TransformComponent>(), 0, 0);

		// Button between 
		const float height = ImGui::GetContentRegionAvail().y;
		if (height > 0.0f)
		{
			ImGui::InvisibleButton(
				"##EntityListDropTarget",
				ImVec2(ImGui::GetContentRegionAvail().x, height)
			);

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY", ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
				{
					if (payload->IsDelivery())
					{
						const Entity& entity = *static_cast<const Entity*>(payload->Data);
						root.GetComponent<TransformComponent>().Attach(entity.GetComponent<TransformComponent>(), root.GetComponent<TransformComponent>().GetChildren().size());
					}
				}

				ImGui::EndDragDropTarget();
			}
		}

		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(2);
		ImGui::EndChild();
		ImGui::End();
	}

	void SceneHierarchy::DrawHierarchy(const TransformComponent& parent, uint32_t rowIndex, uint32_t columIndex)
	{
		const std::vector<Entity>& children = parent.GetChildren();
		columIndex++;
		for (const Entity& child : children)
		{
			const int id = static_cast<int>(child.GetHandle());
			ImGui::PushID(id);

			const ImVec2 rowMin = ImGui::GetCursorScreenPos();
			const float rowWidth = ImGui::GetContentRegionAvail().x;
			const float rowHeight = ImGui::GetFrameHeight();
			const bool isDragging = ImGui::GetDragDropPayload() != nullptr;
			const bool isThisItemSelectedItem = (EditorApplication::Get().GetEditorViewport().m_selectedEntity == child);
			const std::string name = child.GetComponent<NameComponent>().Name;

			// Draw the background for odd rows.
			{
				if (rowIndex % 2 == 1)
				{
					ImDrawList* dl = ImGui::GetWindowDrawList();
					ImVec2 rowMax(rowMin.x + rowWidth, rowMin.y + rowHeight + 6.0f + 3.0f);
					dl->AddRectFilled(ImVec2(rowMin.x, rowMin.y + 3.0f), rowMax, IM_COL32(255, 255, 255, 6));
				}
			}

			// Draw the highlight between items. If the user is dragging an entity above it.
			{
				const bool isHoveringBetweenItems = ImGui::IsMouseHoveringRect(ImGui::GetCursorScreenPos(), ImVec2(ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x, ImGui::GetCursorScreenPos().y + 5.0f), true);

				if (isHoveringBetweenItems && isDragging)
				{
					ImDrawList* dl = ImGui::GetWindowDrawList();
					dl->AddRectFilled(ImVec2(rowMin.x, rowMin.y + 2.0f), ImVec2(rowMin.x + ImGui::GetContentRegionAvail().x, rowMin.y + 4.0f), IM_COL32(50, 50, 120, 225));
				}

				// Check if the user drops the entity between the items.
				{
					ImGui::InvisibleButton("##DropTarget", ImVec2(rowWidth, 6.0f));
					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY", ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
						{
							if (payload->IsDelivery())
							{
								const Entity& entity = *static_cast<const Entity*>(payload->Data);
								const TransformComponent& childUnderDropdown = child.GetComponent<TransformComponent>();
								TransformComponent& childMoving = entity.GetComponent<TransformComponent>();

								if (childUnderDropdown.GetParent() != childMoving.GetParent() && !childUnderDropdown.IsChildOf(childMoving.GetParent()))
								{
									childMoving.GetParent().GetComponent<TransformComponent>().Attach(childMoving, childMoving.GetParent().GetComponent<TransformComponent>().GetChildren().size());
								}
								else
								{
									childUnderDropdown.GetParent().GetComponent<TransformComponent>().AttachAndMoveAboveChild(childUnderDropdown, childMoving);
								}

								ImGui::PopID();
								return;
							}
						}

						ImGui::EndDragDropTarget();
					}
				}
			}

			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().FramePadding.y);

			// Draw the expand/colapse button for this item.
			{
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + columIndex * 20.0f);

				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
				ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.5f));
				if (ImGui::Button(FontAwesome::AngleRight, ImVec2(rowHeight, rowHeight)))
				{
					// TODO add expand/collapse functionality for the hierarchy
				}
				ImGui::PopStyleColor(3);
				ImGui::PopStyleVar();
			}

			// Draw the selectable/nametag for this item.
			{
				ImGui::SameLine(0, 0);
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetStyle().FramePadding.y);

				const bool isHoveringOverItem = ImGui::IsMouseHoveringRect(ImGui::GetCursorScreenPos(), ImVec2(ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x, ImGui::GetCursorScreenPos().y + rowHeight), true);

				const bool isSelectableSelected = (isDragging && isHoveringOverItem) || isThisItemSelectedItem;
				if (isSelectableSelected)
				{
					ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImGui::GetStyleColorVec4(ImGuiCol_Header));
					ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetStyleColorVec4(ImGuiCol_Header));
				}

				ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.0f, 0.5f));
				if (ImGui::Selectable((" " + name).c_str(), isSelectableSelected, 0, ImVec2(0, rowHeight)))
				{
					// Only select the entity if it's not being hovered for a drop operation.
					if (!isDragging)
					{
						EditorApplication::Get().GetEditorViewport().m_selectedEntity = child;
					}
				}
				ImGui::PopStyleVar();

				if (isSelectableSelected)
				{
					ImGui::PopStyleColor(2);
				}
			}

			// Drag source, use previous selectable as the source item.
			{
				if (ImGui::BeginDragDropSource())
				{
					ImGui::SetDragDropPayload("ENTITY", &child, sizeof(child));
					ImGui::TextUnformatted(name.c_str());
					ImGui::EndDragDropSource();
				}
			}

			// Drag Target, use previous selectable as the target item.
			{
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY", ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
					{
						if (payload->IsDelivery())
						{
							const Entity& entity = *static_cast<const Entity*>(payload->Data);
							child.GetComponent<TransformComponent>().Attach(entity.GetComponent<TransformComponent>());
							ImGui::PopID();
							return;
						}
					}

					ImGui::EndDragDropTarget();
				}
			}

			ImGui::PopID();

			rowIndex++;

			// Draw the children of this item.
			TransformComponent& childTransform = child.GetComponent<TransformComponent>();
			DrawHierarchy(childTransform, rowIndex, columIndex);
		}
		columIndex--;
	}
}