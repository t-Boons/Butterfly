#include "Core/Application.hpp"
#include "Renderer/GraphicsAPI.hpp"
#include "Core/Window.hpp"
#include "Renderer/Renderer.hpp"
#include "Renderer/Graph/Blackboard.hpp"
#include "Core/JobSystem.hpp"
#include "Core/Time.hpp"
#include "Scene/Scene.hpp"
#include "Asset/AssetRegistry.hpp"

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

		m_blackboard = new Blackboard();
		m_renderer = new Renderer();
		m_time = new Time();
		m_scene = new Scene();
		m_jobSystem = new JobSystem();
		m_assetRegistry = new AssetRegistry();

		for (auto& layer : m_layers)
		{
			layer->OnInit();
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

		for (auto& layer : m_layers)
		{
			layer->OnTick();
		}

		if (m_window->ShouldClose())
		{
			Quit();
		}
	}

	void Application::Quit()
	{
		m_running = false;
		for (auto& layer : m_layers)
		{
			layer->OnShutdown();
		}

		delete m_jobSystem;
		delete m_scene;
		delete m_time;
		delete m_renderer;
		delete m_window;
		delete m_blackboard;
	}
}