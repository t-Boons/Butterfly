#pragma once
#include "Butterfly.hpp"

#define NUM_DEBUG_VERTICES 4096
#define NUM_CIRCLE_VERTICES 20

namespace Butterfly
{

	class DebugRendererPipelineStage : public IRenderPipelineStage
	{
	public:
		virtual void OnRecordPass(const ViewportRenderEvent& event) override;
		virtual void OnPostRender() override;
	};

	class DebugRenderer
	{
	public:
		void OnPostRender();
		void OnViewportRender(const ViewportRenderEvent& event);
		void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color);
		void DrawBox(const glm::vec3& min, const glm::vec3& max, const glm::vec4& color);
		void DrawSphere(const glm::vec3& center, float radius, const glm::vec4& color);
		void DrawCircle(const glm::vec3& center, float radius, const glm::vec4& color);
		void DrawFrustum(const glm::mat4& projection, const glm::vec4& color);

		struct Vertex
		{
			glm::vec3 Position;
			glm::vec4 Color;
		};

	private:
		friend class DebugRendererPipelineStage;
		std::vector<Vertex> m_debugVertices;
	};
}