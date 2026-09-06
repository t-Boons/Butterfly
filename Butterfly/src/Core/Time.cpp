#include "Core/Time.hpp"

namespace Butterfly
{
	void Time::Init()
	{
		m_startTime = std::chrono::steady_clock::now();
		m_lastTime = m_startTime;
		m_elapsedTime = 0.0f;
		m_deltaTime = 0.0f;
	}

	void Time::Tick()
	{
		const auto currentTime = std::chrono::steady_clock::now();

		const std::chrono::duration<double> delta = currentTime - m_lastTime;
		const std::chrono::duration<double> elapsed = currentTime - m_startTime;

		m_deltaTime = static_cast<float>(delta.count());
		m_elapsedTime = elapsed.count();

		m_lastTime = currentTime;
	}
}