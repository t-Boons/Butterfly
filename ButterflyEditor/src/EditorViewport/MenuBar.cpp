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
						AssetFileMetadata meta;
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

			if (ImGui::BeginMenu("Project Settings"))
			{
				if (ImGui::BeginMenu("ColorSpace"))
				{
					if (ImGui::MenuItem("SRGB"))
					{
						for (auto& viewport : Application::Get().GetRenderer().GetViewports())
						{
							if (ColorspaceCorrectionRenderPipelineStage* stage = viewport.second.RenderPipeline->TryGetStage<ColorspaceCorrectionRenderPipelineStage>())
							{
								stage->SetColorSpace(ColorspaceCorrectionRenderPipelineStage::ColorSpace::SRGB);
							}
						}
					}

					if (ImGui::MenuItem("GammaApproximation"))
					{
						for (auto& viewport : Application::Get().GetRenderer().GetViewports())
						{
							if (ColorspaceCorrectionRenderPipelineStage* stage = viewport.second.RenderPipeline->TryGetStage<ColorspaceCorrectionRenderPipelineStage>())
							{
								stage->SetColorSpace(ColorspaceCorrectionRenderPipelineStage::ColorSpace::GammaApprox);
							}
						}
					}

					if (ImGui::MenuItem("Linear"))
					{
						for (auto& viewport : Application::Get().GetRenderer().GetViewports())
						{
							if (ColorspaceCorrectionRenderPipelineStage* stage = viewport.second.RenderPipeline->TryGetStage<ColorspaceCorrectionRenderPipelineStage>())
							{
								stage->SetColorSpace(ColorspaceCorrectionRenderPipelineStage::ColorSpace::Linear);
							}
						}
					}

					ImGui::EndMenu();
				}

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}
	}
}