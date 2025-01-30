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

		virtual void Init() = 0;

		virtual void Record(Butterfly::GraphBuilder& builder, const Butterfly::Camera& camera) = 0;
	};
}