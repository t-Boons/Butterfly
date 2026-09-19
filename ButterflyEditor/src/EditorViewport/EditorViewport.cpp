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

		if (Application::Get().GetInput().IsKeyDown(BFB_I))
		{
			auto scene = Application::Get().GetScene().m_activeScene;
			entt::entity root = scene->m_rootEntity;

			RefPtr<Scene> newScene = MakeRef<Scene>();
			newScene->m_rootEntity = root;

			scene->CloneTo(*newScene);

			Application::Get().GetScene().m_activeScene = newScene;
		}

		if (Application::Get().GetInput().IsKeyDown(BFB_T))
		{
			m_selectedEntity = Application::Get().GetScene().CreateEntity();

			m_selectedEntity.AddComponent<MeshRendererComponent>();
			m_selectedEntity.AddComponent<SkyboxComponent>();
		}

		if (Application::Get().GetInput().IsKeyDown(BFB_Y))
		{
			Application::Get().GetScene().SaveCurrentScene();
		}
	}

	void EditorViewport::OnRenderImGUI()
	{
		BF_PROFILE_EVENT()

			for (auto& ext : m_viewportExtentions)
			{
				ext->OnRenderImGUI();
			}

		auto view = Application::Get().GetScene().GetEntityRegistry().view<SkyboxComponent>();
		for (const auto& [entity, sb] : view.each())
		{
			if (sb.IsDirty())
			{
				Application::Get().GetRenderer().m_tempSkybox->LoadSkybox(sb);
				sb.ClearDirty();
			}
		}
	}
}