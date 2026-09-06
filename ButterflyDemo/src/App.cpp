#include "App.hpp"
#include "glm/gtx/quaternion.hpp"
#include <numeric>

namespace Butterfly
{
	void SandboxLayer::OnInit()
	{
		BF_PROFILE_EVENT()

			m_imGUi.Init(&Application::Get().GetWindow());
		m_input.Init(&Application::Get().GetWindow());

		Application::Get().GetBlackboard().Register<Camera>(m_spectatorCam.GetCamera(), "ViewCamera");
		Application::Get().GetWindow().Events().OnWindowResize.Subscribe([=](const WindowResizeEvent& ev)
		{
				auto p = m_spectatorCam.GetCamera()->Projection();
				p.AspectRatio = static_cast<float>(ev.Width) / static_cast<float>(ev.Height);
				m_spectatorCam.GetCamera()->SetProjection(p);
		});
	}


	void SandboxLayer::OnTick()
	{
		BF_PROFILE_FRAME("SandboxLayer::OnTick");
		m_spectatorCam.Tick(m_input, Application::Get().GetTime().DeltaTime());

		if (m_input.IsKeyDown(BFB_F11))
		{
			Application::Get().GetWindow().SetFullscreen(!Application::Get().GetWindow().Fullscreen());
		}

		m_input.Poll();
	}

	void SandboxLayer::OnShutdown()
	{
		BF_PROFILE_EVENT()

		FullscreenQuad::ShutDown();
	}
}
