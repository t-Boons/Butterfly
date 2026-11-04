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

	void DrawCell(const std::string& id, const std::string& name, const LibraryIcon& icon,
		bool& doubleClicked, const std::function<void()>& onItem = {})
	{
		BF_PROFILE_EVENT()

		const float cellSize = 96.0f;
		const float margin = 7.0f;

		ImGui::TableNextColumn();
		ImGui::PushID(id.c_str());

		const ImVec2 cellStart = ImGui::GetCursorScreenPos();
		const ImVec2 cellMarginStart = ImVec2(cellStart.x + margin, cellStart.y + margin);
		const ImVec2 cellMarginSize = ImVec2(cellSize - margin * 2, cellSize - margin * 2);

		ImGui::InvisibleButton("##cell", ImVec2(cellSize, cellSize));

		const bool hovered = ImGui::IsItemHovered();
		doubleClicked = hovered && ImGui::IsMouseDoubleClicked(0);

		if (onItem)
		{
			onItem();
		}

		ImDrawList* dl = ImGui::GetWindowDrawList();
		if (hovered)
		{
			dl->AddRect(cellStart, ImVec2(cellStart.x + cellSize, cellStart.y + cellSize),
				IM_COL32(255, 255, 255, 40), 4.0f);
		}

		const float textMargin = 25.0f;
		const ImVec2 iconRectStart = ImVec2(cellMarginStart.x + textMargin * 0.5f, cellMarginStart.y);
		const ImVec2 iconRectSize = ImVec2(cellMarginSize.x - textMargin, cellMarginSize.y - textMargin);

		if (icon.Thumbnail)
		{
			ImGui::SetCursorScreenPos(iconRectStart);
			ImGui::Image(icon.Thumbnail->GetImGUITextureID(), iconRectSize);
		}
		else
		{
			ImFont* iconFont = ImGui::GetIO().Fonts->Fonts[2];
			const float fontSize = iconRectSize.x - 5.0f;
			const ImVec2 iconTextSize = iconFont->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, icon.IconCode);
			const float iconX = cellStart.x + (cellSize - iconTextSize.x) * 0.5f;
			dl->AddText(iconFont, fontSize, ImVec2(iconX, iconRectStart.y), IM_COL32(255, 255, 255, 255), icon.IconCode);
		}

		ImGUIHelpers::TextWrappedCentered(name, ImVec2(cellMarginStart.x, cellMarginStart.y + iconRectSize.y), cellMarginSize.x);
		ImGui::PopID();
	}

	void DrawFileCell(const LibraryIcon& icon, const std::string& name, const AssetFileMetadata& meta, bool& doubleClicked)
	{
		DrawCell(meta.Path.string(), name, icon, doubleClicked, [&]()
			{
				if (ImGui::BeginPopupContextItem())
				{
					if (ImGui::MenuItem("Show in Explorer"))
						system(("explorer.exe /select," + meta.Path.string()).c_str());

					if (ImGui::MenuItem("Delete"))
					{
						EditorApplication::Get().GetEditorViewport().RunBeforeImGuiRender([meta]()
							{
								Application::Get().GetAssetManager().GetAssetRegistry().DeleteFileAndMeta(meta.SourceFileID);
							});
					}
					ImGui::EndPopup();
				}

				if (ImGui::BeginDragDropSource())
				{
					UUID id = meta.RootAssetID;
					ImGui::SetDragDropPayload("ASSET", &id, sizeof(UUID));
					ImGui::Text("Dragging %s", name.c_str());
					ImGui::EndDragDropSource();
				}
			});
	}

	void OpenAsset(const AssetFileMetadata& meta, const std::string& ext)
	{
		if (ext == ".bfscene")
		{
			EditorApplication::Get().GetEditorViewport().RunBeforeImGuiRender([meta]()
				{
					const std::string& scene = FileSystem::ReadText(meta.Path);
					Application::Get().GetScene().LoadSceneFromFile(std::filesystem::path(meta.Path));
					EditorApplication::Get().GetEditorViewport().m_selectedEntity = Entity();
				});
		}
		else if (ext == ".gltf" || ext == ".glb")
		{
			AssetHandle<ModelAsset> handle;
			Application::Get().GetAssetManager().Acquire(meta.RootAssetID, handle);
			ModelAsset* model = Application::Get().GetAssetManager().Resolve(handle);

			std::function<void(ModelNode*, entt::entity)> traverse = [&](ModelNode* node, entt::entity parent)
				{
					entt::entity entity = Application::Get().GetScene().CreateEntity(node->Name);
					auto& tr = Application::Get().GetScene().m_activeScene->GetComponent<TransformComponent>(entity);
					tr.SetWorldMatrix(node->ModelMatrix);

					if (parent != entt::null)
					{
						Application::Get().GetScene().m_activeScene->GetComponent<TransformComponent>(parent).Attach(tr);
					}

					if (node->Mesh)
					{
						MeshRendererComponent& mr = Application::Get().GetScene().m_activeScene->AddComponent<MeshRendererComponent>(entity);
						mr.SetMeshHandle(node->Mesh);
					}

					for (auto& child : node->Children)
					{
						traverse(child.get(), entity);
					}
				};

			traverse(model->RootNode.get(), entt::null);
		}
	}

	AssetLibrary::AssetLibrary()
	{
		m_currentDir = Application::Get().GetAssetManager().GetAssetRegistry().GetRootDirectory();
	}

	AssetLibrary::~AssetLibrary()
	{
	}

	void AssetLibrary::OnTick()
	{

	}

	void AssetLibrary::OnRenderImGUI()
	{
		BF_PROFILE_EVENT()

		auto& registry = Application::Get().GetAssetManager().GetAssetRegistry();
		const std::filesystem::path assetRoot = registry.GetRootDirectory().lexically_normal();

		ImGui::Begin("Asset View");

		ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);

		ImGui::BeginDisabled(m_currentDir == assetRoot);
		if (ImGui::Button(FontAwesome::AngleLeft))
		{
			m_currentDir = m_currentDir.parent_path();
		}
		ImGui::PopFont();

		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::TextDisabled("/%s", m_currentDir.lexically_relative(assetRoot).generic_string().c_str());
		ImGui::Separator();

		std::set<std::string> folders;
		std::vector<const AssetFileMetadata*> files;

		for (const auto& [id, meta] : registry.GetAll())
		{
			const auto parent = meta.Path.lexically_normal().parent_path();

			if (parent == m_currentDir)
			{
				files.push_back(&meta);
				continue;
			}

			const auto sub = parent.lexically_relative(m_currentDir);
			if (!sub.empty() && sub != "." && *sub.begin() != "..")
			{
				folders.insert(sub.begin()->string());
			}
		}

		const float cellSize = 96.0f;
		const float cellPadding = 8.0f;

		ImGui::BeginChild("AssetGrid", ImVec2(0, 0), true);

		const float panelWidth = ImGui::GetContentRegionAvail().x;
		int columnCount = std::max(1, (int)((panelWidth + cellPadding) / (cellSize + cellPadding)));

		if (ImGui::BeginTable("AssetGridTable", columnCount,
			ImGuiTableFlags_SizingFixedSame | ImGuiTableFlags_NoBordersInBody))
		{
			for (const std::string& name : folders)
			{
				LibraryIcon folderIcon;
				folderIcon.IconCode = FontAwesome::Folder;

				bool doubleClicked = false;
				DrawCell("folder:" + name, name, folderIcon, doubleClicked);
				if (doubleClicked)
				{
					m_currentDir /= name;
				}
			}

			for (const AssetFileMetadata* metaPtr : files)
			{
				const auto& meta = *metaPtr;

				if (ThumbnailProcessor::IsSupportedImageType(meta.Path) && !EditorApplication::Get().GetEditorCache().Exists(meta.SourceFileID))
				{
					RefPtr<Thumbnail> thumbnail = ThumbnailProcessor::GetThumbnailFromFile(meta.Path);
					thumbnail = ThumbnailProcessor::Resize(*thumbnail, 64, 64);
					EditorApplication::Get().GetEditorCache().Add<ThumbnailCacheEntry>(meta.SourceFileID, ThumbnailCacheEntry(thumbnail));
				}

				EditorCacheHandle<ThumbnailCacheEntry> entry;
				EditorApplication::Get().GetEditorCache().Get<ThumbnailCacheEntry>(meta.SourceFileID, entry);

				const std::string ext = meta.Path.extension().string();

				LibraryIcon icon;
				icon.IconCode = FontAwesome::File;
				if (ext == ".bfscene") icon.IconCode = FontAwesome::CubeStack;
				else if (ext == ".obj" || ext == ".gltf" || ext == ".glb") icon.IconCode = FontAwesome::Cube;
				if (entry.Loaded) icon.Thumbnail = entry.Data->GetThumbnail();

				bool doubleClicked = false;
				DrawFileCell(icon, meta.Path.filename().string(), meta, doubleClicked);

				if (doubleClicked)
				{
					OpenAsset(meta, ext);
				}
			}

			ImGui::EndTable();
		}

		ImGui::EndChild();
		ImGui::End();
	}
}