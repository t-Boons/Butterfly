#include "Core/GameApplication.hpp"
#include "Core/GameViewport.hpp"

namespace Butterfly::Game
{
	void GameApplication::OnInit()
	{
		BF_PROFILE_EVENT()

		s_instance = this;
		m_viewport = new GameViewport();

		RefPtr<Scene> scene = Scene::DeserializeFromFilePath(Application::Get().GetAssetManager().GetAssetRegistry().GetRootDirectory() / "Untitled Scene 15.bfscene");
		Application::Get().GetScene().SetActiveScene(scene);
	}

	void GameApplication::OnTick()
	{
		BF_PROFILE_EVENT()
	}

	void GameApplication::OnShutdown()
	{
		BF_PROFILE_EVENT()

			delete m_viewport;
	}
}