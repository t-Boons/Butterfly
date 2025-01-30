#pragma once
#include <glfw/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw/glfw3native.h>

#include "Core/Common.hpp"
#include "Renderer/D3D12/Context_DX12.hpp"
#include "EventDispatcher.hpp"

namespace Butterfly
{
	struct WindowResizeEvent
	{
		uint32_t Width;
		uint32_t Height;
	};

	struct KeyEvent
	{
		int Key;
		bool Pressed;
	};

	struct MouseButtonEvent
	{
		int Button;
		bool Pressed;
	};

	struct MousePointerEvent
	{
		int X;
		int Y;
	};

	struct MouseScrollEvent
	{
		int Delta;
	};

	class Window : private NonCopyable
	{
	public:
		struct WindowEvents
		{
			EventDispatcher<WindowResizeEvent> OnWindowResize;
			EventDispatcher<KeyEvent> OnKeyEvent;
			EventDispatcher<MousePointerEvent> OnPointerEvent;
			EventDispatcher<MouseButtonEvent> OnMouseEvent;
			EventDispatcher<MouseScrollEvent> OnScrollEvent;
		};


		Window(const std::string& name, uint32_t width, uint32_t height);
		~Window();

		bool ShouldClose() const;
		void Update();

		uint32_t Width() const { return m_width; }
		uint32_t Height() const { return m_height; }
		std::string Name() const { return m_name; }
		GLFWwindow* GLFWWindow() const { return m_window; }
		BFGraphicsContext& Context() { return *m_context; }
		float Time() const { return m_time; }
		float DeltaTime() const { return m_deltaTime; }
		float AspectRatio() const { return static_cast<float>(m_width) / static_cast<float>(m_height); }

		WindowEvents& Events() { return m_events; }

	private:
		void InitGLFW();
		void BindGLFWCallbacks();
		void BindEvents();

		float m_oldTime;
		float m_deltaTime;
		float m_time;

		uint32_t m_width;
		uint32_t m_height;
		GLFWwindow* m_window;
		std::string m_name;
		HWND m_hwnd;
		BFGraphicsContext* m_context;
		WindowEvents m_events;
	};
}