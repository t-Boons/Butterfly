#pragma once
#include "Butterfly.hpp"
#include "GraphExamples.hpp"
#include "glm/glm.hpp"
#include "../Tools/SpectatorCamera.hpp"
#include "SponzaModel.hpp"

using namespace Butterfly;

namespace GraphExamples
{
	class ForwardSponza : public GraphExample
	{
	public:
		virtual void Init() override;
		void Record(Butterfly::GraphBuilder& builder, const Butterfly::Camera& camera) override;

	private:
		ScopePtr<BFUniformBuffer> m_uniforms;
		SponzaModel* m_sponza;
	};
}