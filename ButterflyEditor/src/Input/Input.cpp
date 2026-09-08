#include "Input.hpp"

namespace Butterfly
{
	void Input::Init(Window* window)
	{
		window->Events().OnKeyEvent.Subscribe([this](KeyEvent event)
			{
				UpdateKey(event.Key, event.Pressed);
			});

		window->Events().OnMouseEvent.Subscribe([this](MouseButtonEvent event)
			{
				UpdateMouseButton(event.Button, event.Pressed);
			});

		window->Events().OnScrollEvent.Subscribe([this](MouseScrollEvent event)
			{
				UpdateScrollDelta(event.Delta);
			});
		window->Events().OnPointerEvent.Subscribe([this](MousePointerEvent event)
			{
				UpdatePointer(event.X, event.Y);
			});
	}

	void Input::Poll()
	{
		Flush();

		m_mouseXDelta = (m_mouseX - m_oldMouseX) * 0.001f;
		m_mouseYDelta = (m_mouseY - m_oldMouseY) * 0.001f;
		m_oldMouseX = m_mouseX;
		m_oldMouseY = m_mouseY;
	}

	bool Input::IsKeyPressed(int key) const
	{
		for (uint32_t i = 0; i < m_keyStateCount; i++)
		{
			if (m_KeyStates[i] == key)
			{
				return true;
			}
		}

		return false;
	}

	bool Input::IsKeyDown(int key) const
	{
		for (uint32_t i = 0; i < m_KeyDownStatesCount; i++)
		{
			if (m_KeyDownStates[i] == key)
			{
				return true;
			}
		}

		return false;
	}

	bool Input::IsKeyUp(int key) const
	{
		for (uint32_t i = 0; i < m_KeyUpStatesCount; i++)
		{
			if (m_KeyUpStates[i] == key)
			{
				return true;
			}
		}

		return false;
	}

	bool Input::IsMousePressed(int button) const
	{
		for (uint32_t i = 0; i < m_mouseStatesCount; i++)
		{
			if (m_mouseStates[i] == button)
			{
				return true;
			}
		}

		return false;
	}

	bool Input::IsMouseDown(int button) const
	{
		for (uint32_t i = 0; i < m_mouseDownStatesCount; i++)
		{
			if (m_mouseDownStates[i] == button)
			{
				return true;
			}
		}

		return false;
	}

	bool Input::IsMouseUp(int button) const
	{
		for (uint32_t i = 0; i < m_mouseUpStatesCount; i++)
		{
			if (m_mouseUpStates[i] == button)
			{
				return true;
			}
		}

		return false;
	}

	void Input::UpdateKey(int key, bool pressed)
	{
		if (pressed)
		{
			if (!IsKeyPressed(key)) m_KeyDownStates[m_KeyDownStatesCount++] = key;
			m_KeyStates[m_keyStateCount++] = key;
		}
		else
		{
			if (IsKeyPressed(key)) m_KeyUpStates[m_KeyUpStatesCount++] = key;

			for (uint32_t i = 0; i < m_keyStateCount; i++)
			{
				if (m_KeyStates[i] == key)
				{
					const int newCount = m_keyStateCount - 1;
					m_KeyStates[i] = m_KeyStates[newCount];
					m_keyStateCount = newCount;
					return;
				}
			}
		}
	}

	void Input::UpdateMouseButton(int button, bool pressed)
	{
		if (pressed)
		{
			if (!IsMousePressed(button)) m_mouseDownStates[m_mouseDownStatesCount++] = button;
			m_mouseStates[m_mouseStatesCount++] = button;
		}
		else
		{
			if (IsMousePressed(button)) m_mouseUpStates[m_mouseUpStatesCount++] = button;

			for (uint32_t i = 0; i < m_mouseStatesCount; i++)
			{
				if (m_mouseStates[i] == button)
				{
					const int newCount = m_mouseStatesCount - 1;
					m_mouseStates[i] = m_mouseStates[newCount];
					m_mouseStatesCount = newCount;
					return;
				}
			}
		}
	}

	void Input::UpdateScrollDelta(int amount)
	{
		m_scrollDelta += amount;
	}

	void Input::UpdatePointer(int x, int y)
	{
		m_mouseX = static_cast<float>(x);
		m_mouseY = static_cast<float>(y);
	}

	void Input::Flush()
	{
		m_KeyDownStatesCount = 0;
		m_KeyUpStatesCount = 0;
		m_mouseDownStatesCount = 0;
		m_mouseUpStatesCount = 0;
		m_scrollDelta = 0;
		m_mouseXDelta = 0;
		m_mouseYDelta = 0;
	}

	void Input::Reset()
	{
		Flush();
		std::fill(std::begin(m_KeyStates), m_KeyStates.begin() + m_keyStateCount, 0);
		std::fill(std::begin(m_mouseStates), m_mouseStates.begin() + m_mouseStatesCount, 0);

		m_keyStateCount = 0;
		m_mouseStatesCount = 0;
	}
}