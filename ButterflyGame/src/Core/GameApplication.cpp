#include "Core/GameApplication.hpp"
#include "Core/GameViewport.hpp"

namespace Butterfly::Game
{
	void GameApplication::OnInit()
	{
		BF_PROFILE_EVENT()

		s_instance = this;
		m_viewport = new GameViewport();

		Application::Get().GetScene().LoadSceneFromFile("C:\\Users\\tygob\\Desktop\\Butterfly\\out\\build\\x64-debug\\ButterflyGame\\Assets\\Untitled Scene 15.bfscene");
	}

	void GameApplication::OnTick()
	{
		BF_PROFILE_EVENT()

			m_viewport->OnTick();
	}

	void GameApplication::OnShutdown()
	{
		BF_PROFILE_EVENT()

			delete m_viewport;
	}
}