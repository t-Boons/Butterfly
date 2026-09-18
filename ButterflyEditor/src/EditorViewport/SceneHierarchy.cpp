#include "EditorViewport/SceneHierarchy.hpp"
#include "EditorViewport/EditorViewport.hpp"
#include "Scene/Registry/IDComponent.hpp"
#include "Core/EditorCache.hpp"


namespace Butterfly
{
	class SceneCollapseHierarchyCacheEntry : public IEditorCacheEntry
	{
	public:
		SceneCollapseHierarchyCacheEntry(const std::unordered_set<UUID>& collapsedHeaderMap)
			: m_collapsedHeaderMap(collapsedHeaderMap)
		{
		}

		std::vector<uint8_t> Serialize() const
		{
			std::vector<std::string> collapsedUUIDs(m_collapsedHeaderMap.size());

			for (const auto& uuid : m_collapsedHeaderMap)
			{
				collapsedUUIDs.push_back(uuid.ToString());
			}

			std::vector<uint8_t> bytes(collapsedUUIDs.size() * UUID::Strlen());
			for (size_t i = 0; i < collapsedUUIDs.size(); ++i)
			{
				const std::string& uuidStr = collapsedUUIDs[i];
				std::copy(uuidStr.begin(), uuidStr.end(), bytes.begin() + i * UUID::Strlen());
			}
			return bytes;
		}

		void Deserialize(const std::vector<uint8_t>& data)
		{
			m_collapsedHeaderMap.clear();
			std::vector<std::string> collapsedUUIDs(data.size() / UUID::Strlen());
			for (size_t i = 0; i < collapsedUUIDs.size(); ++i)
			{
				std::string& uuidStr = collapsedUUIDs[i];
				std::copy(data.begin() + i * UUID::Strlen(), data.begin() + (i + 1) * UUID::Strlen(), uuidStr.begin());
				m_collapsedHeaderMap.insert(UUID::FromString(uuidStr));
			}
		}

	private:
		std::unordered_set<UUID> m_collapsedHeaderMap;
	};

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
		BF_PROFILE_EVENT()

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
						root.GetComponent<TransformComponent>().Attach(entity.GetComponent<TransformComponent>(), root.GetComponent<TransformComponent>().NumChildren());
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
		BF_PROFILE_EVENT()

		columIndex++;

		const uint32_t numChildren = parent.NumChildren();
		for (uint32_t i = 0; i < numChildren; ++i)
		{
			if (numChildren != parent.NumChildren())
			{
				// The number of children has changed, so we need to break out of the loop and start over.
				return;
			}

			const Entity& child = parent.GetChild(i);
			const int id = static_cast<int>(child.GetHandle());
			ImGui::PushID(id);

			const UUID& childUUID = child.GetComponent<IDComponent>().EntityUUID;

			TransformComponent& childTransform = child.GetComponent<TransformComponent>();
			const ImVec2 rowMin = ImGui::GetCursorScreenPos();
			const float rowWidth = ImGui::GetContentRegionAvail().x;
			const float rowHeight = ImGui::GetFrameHeight();
			const bool isDragging = ImGui::GetDragDropPayload() != nullptr;
			const bool isThisItemSelectedItem = (EditorApplication::Get().GetEditorViewport().m_selectedEntity == child);
			const std::string name = child.GetComponent<NameComponent>().Name;

			// Remove this entry from the collapsed header map if the item has no children.
			if (!childTransform.HasChildren() && m_collapsedHeaderMap.find(childUUID) != m_collapsedHeaderMap.end())
			{
				m_collapsedHeaderMap.erase(childUUID);
			}

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
								const TransformComponent& childUnderDropdown = childTransform;
								TransformComponent& childMoving = entity.GetComponent<TransformComponent>();

								if (childUnderDropdown.GetParent() != childMoving.GetParent() && !childUnderDropdown.IsChildOf(childMoving.GetParent()))
								{
									childMoving.GetParent().GetComponent<TransformComponent>().Attach(childMoving, childMoving.GetParent().GetComponent<TransformComponent>().NumChildren());
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

				if (childTransform.HasChildren())
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
					ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.5f));

					const char* graphic = m_collapsedHeaderMap.contains(childUUID) ? FontAwesome::AngleRight : FontAwesome::AngleDown;
					if (ImGui::Button(graphic, ImVec2(rowHeight, rowHeight)))
					{
						if (m_collapsedHeaderMap.contains(childUUID))
						{
							m_collapsedHeaderMap.erase(childUUID);
						}
						else
						{
							m_collapsedHeaderMap.insert(childUUID);
						}
					}
					ImGui::PopStyleColor(3);
					ImGui::PopStyleVar();
				}
				else
				{
					ImGui::Dummy(ImVec2(rowHeight, rowHeight));
				}
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
							childTransform.Attach(entity.GetComponent<TransformComponent>());
							ImGui::PopID();
							return;
						}
					}

					ImGui::EndDragDropTarget();
				}
			}

			ImGui::PopID();

			rowIndex++;

			// Draw the children of this item if not collapsed.
			if (!m_collapsedHeaderMap.contains(childUUID))
			{
				DrawHierarchy(childTransform, rowIndex, columIndex);
			}
		}
		columIndex--;
	}
}