#include "EditorViewport/EditorViewport.hpp"
#include "ImGui/ImGUIHelpers.hpp"
#include "Core/EditorApplication.hpp"
#include "EditorViewport/ThumbnailProcessor.hpp"
#include "EditorViewport/EditorViewportExtention.hpp"
#include "EditorViewport/SceneViewport.hpp"
#include "EditorViewport/SceneHierarchy.hpp"
#include "EditorViewport/EntityProperties.hpp"
#include "EditorViewport/AssetLibrary.hpp"
#include "EditorViewport/MenuBar.hpp"
#include "EditorViewport/GameViewport.hpp"

namespace Butterfly
{
	EditorViewport::EditorViewport()
	{
		m_ImGUIRenderReceiver.Subscribe(Application::Get().GetRenderer().GetImGUIRenderEvent(), BF_BIND_FUNC(&EditorViewport::OnRenderImGUI));

		m_viewportExtentions.push_back(MakeRef<SceneViewport>());
		m_viewportExtentions.push_back(MakeRef<SceneHierarchy>());
		m_viewportExtentions.push_back(MakeRef<EntityProperties>());
		m_viewportExtentions.push_back(MakeRef<AssetLibrary>());
		m_viewportExtentions.push_back(MakeRef<MenuBar>());
		m_viewportExtentions.push_back(MakeRef<GameViewport>());
	}

	void EditorViewport::Tick()
	{
		BF_PROFILE_EVENT()

			for (auto& ext : m_viewportExtentions)
			{
				ext->OnTick();
			}

		if (Application::Get().GetInput().IsKeyDown(BFB_F11))
		{
			Application::Get().GetWindow().SetFullscreen(!Application::Get().GetWindow().Fullscreen());
		}

		if (Application::Get().GetInput().IsKeyDown(BFB_Y))
		{
			Application::Get().GetScene().SaveCurrentScene();
		}
	}

	void EditorViewport::RunBeforeImGuiRender(const std::function<void()>& func)
	{
		m_runBeforeImGuiRender.push(func);
	}

	void EditorViewport::OnRenderImGUI()
	{
		BF_PROFILE_EVENT()

		while (!m_runBeforeImGuiRender.empty())
		{
			m_runBeforeImGuiRender.front()();
			m_runBeforeImGuiRender.pop();
		}

		for (auto& ext : m_viewportExtentions)
			{
			ext->OnRenderImGUI();
		}


		auto view = Application::Get().GetScene().GetEntityRegistry().view<SkyboxComponent>();
		auto first = view.begin();
		if (first != view.end())
		{
			SkyboxComponent& sb = view.get<SkyboxComponent>(*first);

			if (sb.IsDirty())
			{
				sb.ClearDirty();

				for (const auto& [handle, viewport] : Application::Get().GetRenderer().GetViewports())
				{
					SkyboxRenderPipelineStage* renderPipeline = viewport.RenderPipeline->TryGetStage<SkyboxRenderPipelineStage>();
					if (renderPipeline)
					{
						renderPipeline->LoadSkybox(sb);
					}
				}
			}

		}
		else
		{
			for (const auto& [handle, viewport] : Application::Get().GetRenderer().GetViewports())
			{
				SkyboxRenderPipelineStage* renderPipeline = viewport.RenderPipeline->TryGetStage<SkyboxRenderPipelineStage>();
				if (renderPipeline && renderPipeline->IsSkyboxLoaded())
				{
					renderPipeline->UnloadSkybox();
				}
			}
		}
	}
}