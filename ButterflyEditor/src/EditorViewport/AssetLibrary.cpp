#include "EditorViewport/AssetLibrary.hpp"
#include "Core/EditorApplication.hpp"
#include "EditorViewport/ThumbnailProcessor.hpp"

namespace Butterfly
{
	AssetLibrary::AssetLibrary()
	{
	}

	AssetLibrary::~AssetLibrary()
	{
	}

	void AssetLibrary::OnTick()
	{

	}

	void AssetLibrary::OnRenderImGUI()
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