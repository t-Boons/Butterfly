#include "Core/GameApplication.hpp"
#include "Core/GameViewport.hpp"

namespace Butterfly::Game
{
	void GameApplication::OnInit()
	{
		BF_PROFILE_EVENT()

		s_instance = this;
		m_viewport = new GameViewport();

		Application::Get().GetScene().LoadSceneFromFile(Application::Get().GetAssetManager().GetAssetRegistry().GetRootDirectory() / "Untitled Scene 15.bfscene");
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