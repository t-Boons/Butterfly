#pragma once
#include "Core/Common.hpp"
#include <chrono>

namespace Butterfly
{
	class Time : public NonCopyable
	{
	public:
		Time();

		void Tick();
		double Elapsed() const { return m_elapsedTime; }
		float DeltaTime() const { return m_deltaTime; }

	private:
		std::chrono::steady_clock::time_point m_startTime;
		std::chrono::steady_clock::time_point m_lastTime;

		float m_deltaTime;
		double m_elapsedTime;
	};
}