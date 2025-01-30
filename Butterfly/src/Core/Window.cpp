#include "Core/Window.hpp"
#include "Utils/Profiler.hpp"

namespace Butterfly
{
	Window::Window(const std::string& name, uint32_t width, uint32_t height)
		: m_width(width), m_height(height), m_name(name)
	{
		BF_PROFILE_EVENT()
		InitGLFW();
		BindGLFWCallbacks();
		BindEvents();
	}

	Window::~Window()
	{
		FREE(m_context);
	}

	bool Window::ShouldClose() const
	{
		return glfwWindowShouldClose(m_window);
	}

	void Window::Update()
	{
		BF_PROFILE_EVENT()

		m_oldTime = m_time;
		m_time = static_cast<float>(glfwGetTime());
		m_deltaTime = m_time - m_oldTime;
		glfwPollEvents();
	}

	void Window::InitGLFW()
	{
		if (!glfwInit())
		{
			Throw("Failed to initialize GLTF.");
		}

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		m_window = glfwCreateWindow(m_width, m_height, m_name.c_str(), nullptr, nullptr);
		if (!m_window)
		{
			glfwTerminate();
			Throw("Failed to initialize GLTF Window.");
		}

		glfwShowWindow(m_window);
		glfwMakeContextCurrent(m_window);
		m_hwnd = glfwGetWin32Window(m_window);
		m_context = new BFGraphicsContext(m_hwnd);
	}

	void Window::BindGLFWCallbacks()
	{
		BF_PROFILE_EVENT()

		glfwSetWindowUserPointer(m_window, &m_events);

		glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, int width, int height)
			{
				WindowResizeEvent event;
				event.Width = static_cast<uint32_t>(width);
				event.Height = static_cast<uint32_t>(height);

				WindowEvents* data = static_cast<WindowEvents*>(glfwGetWindowUserPointer(window));
				data->OnWindowResize.Broadcast(event);
			});

		glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int, int action, int)
			{
				if (action != 2)
				{
					KeyEvent event;
					event.Key = key;
					event.Pressed = action;

					WindowEvents* data = static_cast<WindowEvents*>(glfwGetWindowUserPointer(window));
					data->OnKeyEvent.Broadcast(event);
				}
			});

		glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double deltaX, double deltaY)
			{
				MousePointerEvent event;
				event.X = static_cast<int>(deltaX);
				event.Y = static_cast<int>(deltaY);

				WindowEvents* data = static_cast<WindowEvents*>(glfwGetWindowUserPointer(window));
				data->OnPointerEvent.Broadcast(event);
			});

		glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, int button, int action, int)
			{
				MouseButtonEvent event;
				event.Button = button;
				event.Pressed = action;

				WindowEvents* data = static_cast<WindowEvents*>(glfwGetWindowUserPointer(window));
				data->OnMouseEvent.Broadcast(event);
			});

		glfwSetScrollCallback(m_window, [](GLFWwindow* window, double, double deltaY)
			{
				MouseScrollEvent event;
				event.Delta = static_cast<int>(deltaY);

				WindowEvents* data = static_cast<WindowEvents*>(glfwGetWindowUserPointer(window));
				data->OnScrollEvent.Broadcast(event);
			});

	}

	void Window::BindEvents()
	{
		BF_PROFILE_EVENT()

		m_events.OnWindowResize.Subscribe([&](const WindowResizeEvent& ev)
			{
				m_width = ev.Width;
				m_height = ev.Height;
			});

		m_events.OnWindowResize.Subscribe([&](const WindowResizeEvent& ev)
			{
				m_context->Resize(ev.Width, ev.Height);
			});
	}
}