#pragma once
#include "Butterfly.hpp"
#include "glm/glm.hpp"

#include "../Tools/Camera.hpp"

namespace GraphExamples
{
	class GraphExample : private Butterfly::NonCopyable
	{
	public:
		virtual ~GraphExample() = default;

		virtual void Init(Butterfly::Blackboard* blackBoard, Butterfly::Window* window)
		{
			m_blackBoard = blackBoard;
			m_window = window;
		}

		virtual void Record(Butterfly::GraphBuilder& builder, const Butterfly::Camera& camera) = 0;

	protected:
		Butterfly::Blackboard* m_blackBoard;
		Butterfly::Window* m_window;
	};
}