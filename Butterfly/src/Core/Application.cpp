#include "Core/Application.hpp"

namespace Butterfly
{
	void Application::Start()
	{
		m_running = true;
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