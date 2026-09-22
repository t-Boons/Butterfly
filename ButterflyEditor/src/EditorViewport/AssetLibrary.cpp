#include "EditorViewport/AssetLibrary.hpp"
#include "Core/EditorApplication.hpp"
#include "EditorViewport/ThumbnailProcessor.hpp"
#include "EditorViewport/EditorViewport.hpp"
#include "ImGUI/FontAwesomeIcons.hpp"
#include "ImGUI/ImGUIHelpers.hpp"

namespace Butterfly
{
	struct LibraryIcon
	{
		const char* IconCode;
		RefPtr<Thumbnail> Thumbnail;
	};

	void DrawAssetLibraryCell(const LibraryIcon& iconType, const std::string& name, const UUID& id, const AssetMetadata& meta, bool& doubleClicked)
	{
		const float cellSize = 96.0f;
		const float cellPadding = 8.0f;
		const float margin = 7.0f;

		ImGui::TableNextColumn();
		ImGui::PushID(static_cast<int>(std::hash<Butterfly::UUID>()(id)));

		const ImVec2 cellStart = ImGui::GetCursorScreenPos();
		const ImVec2 cellMarginStart = ImVec2(cellStart.x + margin, cellStart.y + margin);
		const ImVec2 cellMarginSize = ImVec2(cellSize - margin * 2, cellSize - margin * 2);

		ImGui::InvisibleButton("##cell", ImVec2(cellSize, cellSize));

		const bool hovered = ImGui::IsItemHovered();
		doubleClicked = hovered && ImGui::IsMouseDoubleClicked(0);

		ImDrawList* dl = ImGui::GetWindowDrawList();
		ImU32 iconColor = ImColor(10, 20, 50);

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
			ImGui::Text("Dragging %s", name.c_str());

			ImGui::EndDragDropSource();
		}

		if (hovered)
		{
			dl->AddRect(ImVec2(cellStart.x, cellStart.y), ImVec2(cellStart.x + cellSize, cellStart.y + cellSize), IM_COL32(255, 255, 255, 40), 4.0f);
		}

		const float textMargin = 25.0f;
		const ImVec2 iconRectStart = ImVec2(cellMarginStart.x + textMargin * 0.5f, cellMarginStart.y);
		const ImVec2 iconRectSize = ImVec2(cellMarginSize.x - textMargin, cellMarginSize.y - textMargin);



		if (iconType.Thumbnail)
		{
			ImGui::SetCursorScreenPos(iconRectStart);
			ImGui::Image(iconType.Thumbnail->GetImGUITextureID(), iconRectSize);
		}
		else
		{
			ImFont* iconFont = ImGui::GetIO().Fonts->Fonts[2];
			const float fontSize = iconRectSize.x - 5.0f;
			const ImVec2 iconTextSize = iconFont->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, iconType.IconCode);
			const float iconX = cellStart.x + (cellSize - iconTextSize.x) * 0.5f;
			dl->AddText(iconFont, fontSize, ImVec2(iconX, iconRectStart.y), IM_COL32(255, 255, 255, 255), iconType.IconCode);
		}

		ImGUIHelpers::TextWrappedCentered(name, ImVec2(cellMarginStart.x, cellMarginStart.y + iconRectSize.y), cellMarginSize.x);
		ImGui::PopID();
	}

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
		const float margin = 7.0f;

		ImGui::BeginChild("AssetGrid", ImVec2(0, 0), true);

		const float panelWidth = ImGui::GetContentRegionAvail().x;
		int columnCount = (int)((panelWidth + cellPadding) / (cellSize + cellPadding));
		if (columnCount < 1) columnCount = 1;

		if (ImGui::BeginTable("AssetGridTable", columnCount, ImGuiTableFlags_SizingFixedSame | ImGuiTableFlags_NoBordersInBody))
		{
			for (auto& [id, meta] : Application::Get().GetAssetManager().GetAssetRegistry().GetAll())
			{
				if (ThumbnailProcessor::IsSupportedImageType(meta.Path) && !EditorApplication::Get().GetEditorCache().Exists(meta.ID))
				{
					RefPtr<Thumbnail> thumbnail = ThumbnailProcessor::GetThumbnailFromFile(meta.Path);
					thumbnail = ThumbnailProcessor::Resize(*thumbnail, 64, 64);

					ThumbnailCacheEntry entry(thumbnail);
					EditorApplication::Get().GetEditorCache().Add<ThumbnailCacheEntry>(meta.ID, entry);
				}

				ThumbnailCacheEntry entry;
				EditorApplication::Get().GetEditorCache().Get<ThumbnailCacheEntry>(meta.ID, entry);

				LibraryIcon icon;
				icon.Thumbnail = entry.GetThumbnail();
				icon.IconCode = FontAwesome::File;
				if (meta.Extention == ".bfscene")
				{
					icon.IconCode = FontAwesome::CubeStack;
				}
				else if (meta.Extention == ".obj")
				{
					icon.IconCode = FontAwesome::Cube;
				}

				bool doubleClicked = false;
				DrawAssetLibraryCell(icon, std::filesystem::path(meta.Path).filename().string(), meta.ID, meta, doubleClicked);

				if (doubleClicked)
				{
					if (meta.Extention == ".bfscene")
					{
						const std::string& scene = FileSystem::ReadText(meta.Path);
						Application::Get().GetScene().LoadSceneFromFile(std::filesystem::path(meta.Path));
						EditorApplication::Get().GetEditorViewport().m_selectedEntity = Entity();
						ImGui::PopID();
						ImGui::EndTable();
						ImGui::EndChild();
						ImGui::End();
						return;
					}
				}
			}

			ImGui::EndTable();
		}

		ImGui::EndChild();
		ImGui::End();
	}
}