#include "Core/Application.hpp"
#include "Renderer/GraphicsAPI.hpp"
#include "Core/Window.hpp"
#include "Renderer/Renderer.hpp"
#include "Renderer/Graph/Blackboard.hpp"


namespace Butterfly
{
	void Application::Init()
	{
		BF_PROFILE_EVENT()

		m_running = true;
		m_thisApp = this;

		bool useDebug = true;
#ifdef NDEBUG
		useDebug = false;
#endif

		GraphicsAPI::Init(GraphicsAPIType::D3D12, useDebug);
		m_window = new Window("Butterfly Renderer", 1280, 720);

		m_blackboard = new Blackboard();

		m_renderer = new Renderer();
		m_renderer->Init();


		for (auto& layer : m_layers)
		{
			layer->OnInit();
		}

		while (m_running)
		{
			Update();
		}
	}

	void Application::Update()
	{
		if (m_window->ShouldClose())
		{
			Quit();
		}

		m_window->Update();
		m_renderer->Render();

		for (auto& layer : m_layers)
		{
			layer->OnTick();
		}
	}

	void Application::Quit()
	{
		m_running = false;
		for (auto& layer : m_layers)
		{
			layer->OnShutdown();
		}
	}
}