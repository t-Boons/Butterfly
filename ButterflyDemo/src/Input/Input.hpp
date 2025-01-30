#pragma once
#include "Butterfly.hpp"
#include <queue>
#include <thread>
#include <array>
#include "WindowsKeyCodes.hpp"

#define EVENTX_MAX 8
#define BUTTONS_PRESSED_MAX 64
#define MOUSE_BUTTONS_PRESSED_MAX 8

namespace Butterfly
{
	class Input
	{
	public:
		void Init(Window* window);
		void Poll();
		bool IsKeyPressed(int key) const;
		bool IsKeyDown(int key) const;
		bool IsKeyUp(int key) const;
		bool IsMousePressed(int button) const;
		bool IsMouseDown(int button) const;
		bool IsMouseUp(int button) const;
		float MouseXDelta() const { return m_mouseXDelta; }
		float MouseYDelta() const { return m_mouseYDelta; }
		float MouseX() const { return m_mouseX; }
		float MouseY() const { return m_mouseY; }
		float Scroll() const { return static_cast<float>(m_scrollDelta); }

	private:
		void UpdateKey(int key, bool pressed);
		void UpdateMouseButton(int button, bool pressed);
		void UpdateScrollDelta(int amount);
		void UpdatePointer(int x, int y);
		void Flush();
		void Reset();

	private:
		uint32_t m_keyStateCount = 0;
		uint32_t m_KeyDownStatesCount = 0;
		uint32_t m_KeyUpStatesCount = 0;

		std::array<int, BUTTONS_PRESSED_MAX> m_KeyStates;
		std::array<int, BUTTONS_PRESSED_MAX> m_KeyDownStates;
		std::array<int, BUTTONS_PRESSED_MAX> m_KeyUpStates;

		uint32_t m_mouseStatesCount = 0;
		uint32_t m_mouseDownStatesCount = 0;
		uint32_t m_mouseUpStatesCount = 0;

		std::array<int, MOUSE_BUTTONS_PRESSED_MAX> m_mouseStates;
		std::array<int, MOUSE_BUTTONS_PRESSED_MAX> m_mouseDownStates;
		std::array<int, MOUSE_BUTTONS_PRESSED_MAX> m_mouseUpStates;

		float m_mouseXDelta = 0;
		float m_mouseYDelta = 0;
		float m_mouseX = 0;
		float m_mouseY = 0;
		float m_oldMouseX = 0;
		float m_oldMouseY = 0;
		int m_scrollDelta = 0;
	};
}