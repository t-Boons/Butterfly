#include "Core/GameApplication.hpp"
#include "Core/GameViewport.hpp"

namespace Butterfly::Game
{
	void GameApplication::OnInit()
	{
		BF_PROFILE_EVENT()

		s_instance = this;
		m_viewport = new GameViewport();

		RefPtr<Level> level = Level::DeserializeFromFilePath(Application::Get().GetAssetManager().GetAssetRegistry().GetRootDirectory() / "Untitled Level 15.bflevel");
		Application::Get().GetLevel().SetActiveLevel(level);
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
