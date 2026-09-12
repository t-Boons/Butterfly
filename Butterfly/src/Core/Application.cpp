#include "Core/Application.hpp"
#include "Renderer/GraphicsAPI.hpp"
#include "Core/Window.hpp"
#include "Renderer/Renderer.hpp"
#include "Renderer/Graph/Blackboard.hpp"
#include "Core/JobSystem.hpp"
#include "Core/Time.hpp"
#include "Scene/Scene.hpp"
#include "Asset/AssetManager.hpp"
#include "Input/Input.hpp"

namespace Butterfly
{
	void Application::Init()
	{
		BF_PROFILE_EVENT()

		m_running = true;
		s_instance = this;

		bool useDebug = true;
#ifdef NDEBUG
		useDebug = false;
#endif

		GraphicsAPI::Init(GraphicsAPIType::D3D12, useDebug);
		m_window = new Window("Butterfly Renderer", 1280, 720);

		m_renderer = new Renderer();
		m_time = new Time();
		m_scene = new Scene();
		m_jobSystem = new JobSystem();
		m_assetManager = new AssetManager();

		m_input = new Input();
		m_input->Init(m_window);

		if (m_applicationExtention)
		{
			m_applicationExtention->OnInit();
		}

		while (m_running)
		{
			Tick();
		}
	}

	void Application::Tick()
	{
		m_window->Tick();
		m_renderer->Render();

		m_time->Tick();
		m_scene->Tick();

		if (m_applicationExtention)
		{
			m_applicationExtention->OnTick();
		}

		if (m_window->ShouldClose())
		{
			Quit();
		}
		m_input->Poll();
	}

	void Application::Quit()
	{
		m_running = false;
		if (m_applicationExtention)
		{
			m_applicationExtention->OnShutdown();
		}

		delete m_applicationExtention;
		delete m_jobSystem;
		delete m_scene;
		delete m_time;
		delete m_renderer;
		delete m_window;
	}
}