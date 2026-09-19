#include "EditorViewport/AssetLibrary.hpp"
#include "Core/EditorApplication.hpp"
#include "EditorViewport/ThumbnailProcessor.hpp"
#include "EditorViewport/EditorViewport.hpp"
#include "ImGUI/FontAwesomeIcons.hpp"
#include "ImGUI/ImGUIHelpers.hpp"

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
		const float margin = 7.0f;

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
				ImGui::PushID(static_cast<int>(std::hash<Butterfly::UUID>()(id)));

				const ImVec2 cellStart = ImGui::GetCursorScreenPos();
				const ImVec2 cellMarginStart = ImVec2(cellStart.x + margin, cellStart.y + margin);
				const ImVec2 cellMarginSize = ImVec2(cellSize - margin * 2, cellSize - margin * 2);

				ImGui::InvisibleButton("##cell", ImVec2(cellSize, cellSize));

				const bool hovered = ImGui::IsItemHovered();
				const bool doubleClicked = hovered && ImGui::IsMouseDoubleClicked(0);

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
					ImGui::Text("Dragging %s", std::filesystem::path(meta.Path).filename().string().c_str());

					ImGui::EndDragDropSource();
				}

				if (hovered)
				{
					dl->AddRect(ImVec2(cellStart.x, cellStart.y), ImVec2(cellStart.x + cellSize, cellStart.y + cellSize), IM_COL32(255, 255, 255, 40), 4.0f);
				}

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

				if (ThumbnailProcessor::IsSupportedImageType(meta.Path) && !EditorApplication::Get().GetEditorCache().Exists(meta.ID))
				{
					RefPtr<Thumbnail> thumbnail = ThumbnailProcessor::GetThumbnailFromFile(meta.Path);
					thumbnail = ThumbnailProcessor::Resize(*thumbnail, 64, 64);

					ThumbnailCacheEntry entry(thumbnail);
					EditorApplication::Get().GetEditorCache().Add<ThumbnailCacheEntry>(meta.ID, entry);
				}

				const float textMargin = 25.0f;
				const ImVec2 iconRectStart = ImVec2(cellMarginStart.x + textMargin * 0.5f, cellMarginStart.y);
				const ImVec2 iconRectSize = ImVec2(cellMarginSize.x - textMargin, cellMarginSize.y - textMargin);

				ThumbnailCacheEntry cacheEntry;
				if (EditorApplication::Get().GetEditorCache().Get<ThumbnailCacheEntry>(meta.ID, cacheEntry))
				{
					RefPtr<Thumbnail> thumbnail = cacheEntry.GetThumbnail();

					ImGui::SetCursorScreenPos(iconRectStart);
					ImGui::Image(thumbnail->GetImGUITextureID(), iconRectSize);
				}
				else
				{
					ImFont* iconFont = ImGui::GetIO().Fonts->Fonts[2];
					float fontSize = iconRectSize.x - 5.0f;
					
					const char* icon = FontAwesome::File;

					if (meta.Extention == ".bfscene")
					{
						icon = FontAwesome::CubeStack;
					}
					else if (meta.Extention == ".obj")
					{
						icon = FontAwesome::Cube;
					}
					
					ImVec2 iconTextSize = iconFont->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, icon);
					
					float iconX = cellStart.x + (cellSize - iconTextSize.x) * 0.5f;
					dl->AddText(iconFont, fontSize, ImVec2(iconX, iconRectStart.y), IM_COL32(255, 255, 255, 255), icon);
				}

				const std::string name = std::filesystem::path(meta.Path).filename().string();

				ImGUIHelpers::TextWrappedCentered(name, ImVec2(cellMarginStart.x, cellMarginStart.y + iconRectSize.y), cellMarginSize.x);
				ImGui::PopID();
			}

			ImGui::EndTable();
		}

		ImGui::EndChild();
		ImGui::End();
	}
}