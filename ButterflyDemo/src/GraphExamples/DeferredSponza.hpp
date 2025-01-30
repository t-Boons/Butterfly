#pragma once
#include "Butterfly.hpp"
#include "GraphExamples.hpp"
#include "glm/glm.hpp"
#include "SponzaModel.hpp"

using namespace Butterfly;

namespace GraphExamples
{
	class DeferredSponza : public GraphExample
	{
	public:
		virtual void Init(Butterfly::Blackboard* blackBoard, Butterfly::Window* window) override;
		void Record(Butterfly::GraphBuilder& builder, const Butterfly::Camera& camera) override;

	private:
		ScopePtr<BFUniformBuffer> m_uniforms;
		SponzaModel* m_sponza;
	};
}