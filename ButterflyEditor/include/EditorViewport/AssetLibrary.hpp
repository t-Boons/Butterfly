#pragma once
#include "Butterfly.hpp"
#include "EditorViewport/EditorViewportExtention.hpp"

namespace Butterfly
{
	class Thumbnail;
	class AssetLibrary : public IEditorViewportExtention
	{
	public:
		AssetLibrary();
		~AssetLibrary();

		virtual void OnTick() override;
		virtual void OnRenderImGUI() override;

	private:
		struct LibraryIcon
		{
			const char *IconCode;
			RefPtr<Thumbnail> Thumbnail;
		};

		void DrawFileCell(const LibraryIcon& icon, const std::string& name, const AssetFileMetadata& meta, bool& doubleClicked);
		void OpenAsset(const AssetFileMetadata& meta, const std::string& ext);
		void DrawCell(const std::string& id, const std::string& name, const LibraryIcon& icon, bool& doubleClicked, const std::function<void()>& onItem = {}, bool renaming = false, std::string* renameBuffer = nullptr, const std::function<void()>& onRename = {});
		
		std::filesystem::path m_currentDir;
		std::string m_renameBuffer;
		UUID m_renameID;
		bool m_focusRename = false;
	};
}