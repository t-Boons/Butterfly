#include "EditorViewport/MenuBar.hpp"

namespace Butterfly
{
	MenuBar::MenuBar()
	{
	}

	MenuBar::~MenuBar()
	{
	}

	void MenuBar::OnTick()
	{
	}

	void MenuBar::OnRenderImGUI()
	{
		BF_PROFILE_EVENT()

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
							BF_LOG_INFO("Imported file");
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

			ImGui::EndMainMenuBar();
		}
	}
}