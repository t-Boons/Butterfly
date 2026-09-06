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


		Transform tr;
		MeshRenderer mr;

		mr.LoadTestModel();

		model = Application::Get().GetScene().CreateEntity();

		model.AddComponent<Transform>(tr);
		model.AddComponent<MeshRenderer>(std::move(mr));

	}

	void SandboxLayer::OnTick()
	{
		BF_PROFILE_FRAME("SandboxLayer::OnTick");
		m_spectatorCam.Tick(m_input, Application::Get().GetTime().DeltaTime());

		if (m_input.IsKeyDown(BFB_F11))
		{
			Application::Get().GetWindow().SetFullscreen(!Application::Get().GetWindow().Fullscreen());
		}


		if (m_input.IsKeyPressed(BFB_R))
		{
			m_modelMovementTime += Application::Get().GetTime().DeltaTime();

			Transform& tr = model.GetComponent<Transform>();

			glm::vec3 position = tr.GetPosition();
			position.y = glm::sin(m_modelMovementTime * 3);
			tr.SetPosition(position);

			tr.SetRotation(glm::quat(glm::vec3(0.0f, m_modelMovementTime * 5, 0.0f)));
		}

		m_input.Poll();
	}

	void SandboxLayer::OnShutdown()
	{
		BF_PROFILE_EVENT()

		FullscreenQuad::ShutDown();
	}
}
