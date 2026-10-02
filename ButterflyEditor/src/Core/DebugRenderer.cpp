#include "Core/DebugRenderer.hpp"
#include "Core/EditorApplication.hpp"

namespace Butterfly
{
	void DebugRendererPipelineStage::OnRecordPass(const ViewportRenderEvent& event)
	{
		struct DebugRendererPassData
		{
			BFStructuredBuffer* VertexBuffer;
			uint32_t NumVertices;
		};

		BFStructuredBufferDesc desc;
		desc.HeapType = BFHeapType::Upload;
		desc.NumElements = NUM_DEBUG_VERTICES;
		desc.DebugName = "DebugRendererVertexBuffer";
		desc.Stride = sizeof(DebugRenderer::Vertex);

		BFStructuredBuffer* vertexBuffer = event.Builder.CreateTransientStructuredBuffer("DebugRendererVertexBuffer", desc);

		const std::vector<DebugRenderer::Vertex>& vertices = EditorApplication::Get().GetDebugRenderer().m_debugVertices;


		uint32_t numBytes = vertices.size() * sizeof(DebugRenderer::Vertex);
		if (vertices.size() > vertexBuffer->NumElements())
		{
			BF_CORE_LOG_WARN("DebugRenderer: Too many vertices to render. Max: %u, Current: %u", vertexBuffer->NumElements(), vertices.size());
			numBytes = vertexBuffer->NumElements() * sizeof(DebugRenderer::Vertex);
		}

		vertexBuffer->Write(vertices.data(), numBytes);

		DebugRendererPassData* data = event.Builder.AllocParameters<DebugRendererPassData>();
		data->VertexBuffer = vertexBuffer;
		data->NumVertices = static_cast<uint32_t>(vertices.size());

		event.Builder.AddPass<DebugRendererPassData>("DebugRenderer", [&](const DebugRendererPassData& data, D3D12CommandList& list)
			{
				BF_PROFILE_EVENT_DYNAMIC("Forward Model pass");

				list.List()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
				GraphicsCommands::SetRenderTargets(list, { &event.Viewport.GetRenderTarget() }, &event.Viewport.GetDepthStencil());

				GraphicsCommands::SetFullscreenViewportAndRect(list, event.Viewport.GetRenderTarget().Width(), event.Viewport.GetRenderTarget().Height());

				BFPipelineBuilder psoBuilder;
				psoBuilder.PrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);
				psoBuilder.RenderTargetFormats({ DXGI_FORMAT_R8G8B8A8_UNORM });
				psoBuilder.DepthStencilFormat(DXGI_FORMAT_D24_UNORM_S8_UINT);
				psoBuilder.VertexShader(BFShaderCache::GetOrCreate(L"assets/Shaders/DebugLines_vert.hlsl", ShaderType::Vertex));
				psoBuilder.PixelShader(BFShaderCache::GetOrCreate(L"assets/Shaders/DebugLines_frag.hlsl", ShaderType::Pixel));
				psoBuilder.CullingMode(D3D12_CULL_MODE_BACK);
				psoBuilder.DepthEnable(true);
				psoBuilder.DepthWriteMask(D3D12_DEPTH_WRITE_MASK_ZERO);
				psoBuilder.DepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);
				psoBuilder.EnableBlending();

				list.List()->SetPipelineState(psoBuilder.Create().GetHW());

				ShaderVariables()
					.Add(event.Viewport.Uniforms->GetView(HASH("CameraData"))->View())
					.Add(data.VertexBuffer->SRV().View())
					.Submit(list);

				list.DrawInstanced(data.NumVertices, 1, 0, 0);
			});
	}

	void DebugRendererPipelineStage::OnPostRender()
	{
		EditorApplication::Get().GetDebugRenderer().OnPostRender();
	}


	void DebugRenderer::OnPostRender()
	{
		m_debugVertices.clear();
	}

	void DebugRenderer::DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color)
	{
		m_debugVertices.push_back({ start, color });
		m_debugVertices.push_back({ end, color });
	}

	void DebugRenderer::DrawBox(const glm::vec3& min, const glm::vec3& max, const glm::vec4& color)
	{
		DrawLine({ min.x, min.y, min.z }, { max.x, min.y, min.z }, color);
		DrawLine({ min.x, max.y, min.z }, { max.x, max.y, min.z }, color);
		DrawLine({ min.x, min.y, max.z }, { max.x, min.y, max.z }, color);
		DrawLine({ min.x, max.y, max.z }, { max.x, max.y, max.z }, color);

		DrawLine({ min.x, min.y, min.z }, { min.x, max.y, min.z }, color);
		DrawLine({ max.x, min.y, min.z }, { max.x, max.y, min.z }, color);
		DrawLine({ min.x, min.y, max.z }, { min.x, max.y, max.z }, color);
		DrawLine({ max.x, min.y, max.z }, { max.x, max.y, max.z }, color);

		DrawLine({ min.x, min.y, min.z }, { min.x, min.y, max.z }, color);
		DrawLine({ max.x, min.y, min.z }, { max.x, min.y, max.z }, color);
		DrawLine({ min.x, max.y, min.z }, { min.x, max.y, max.z }, color);
		DrawLine({ max.x, max.y, min.z }, { max.x, max.y, max.z }, color);
	}

	void DebugRenderer::DrawSphere(const glm::vec3& center, float radius, const glm::vec4& color)
	{
		for (int i = 0; i < 360; i += 360 / NUM_CIRCLE_VERTICES)
		{
			float rad = glm::radians((float)i);
			float nextRad = glm::radians((float)(i + 360 / NUM_CIRCLE_VERTICES));
			glm::vec3 p1 = center + glm::vec3(radius * cos(rad), radius * sin(rad), 0.0f);
			glm::vec3 p2 = center + glm::vec3(radius * cos(nextRad), radius * sin(nextRad), 0.0f);
			DrawLine(p1, p2, color);
			p1 = center + glm::vec3(0.0f, radius * cos(rad), radius * sin(rad));
			p2 = center + glm::vec3(0.0f, radius * cos(nextRad), radius * sin(nextRad));
			DrawLine(p1, p2, color);
			p1 = center + glm::vec3(radius * cos(rad), 0.0f, radius * sin(rad));
			p2 = center + glm::vec3(radius * cos(nextRad), 0.0f, radius * sin(nextRad));
			DrawLine(p1, p2, color);
		}
	}


	void DebugRenderer::DrawCircle(const glm::vec3& center, float radius, const glm::vec4& color)
	{
		for (int i = 0; i < 360; i += 360 / NUM_CIRCLE_VERTICES)
		{
			float rad = glm::radians((float)i);
			float nextRad = glm::radians((float)(i + 360 / NUM_CIRCLE_VERTICES));
			glm::vec3 p1 = center + glm::vec3(radius * cos(rad), radius * sin(rad), 0.0f);
			glm::vec3 p2 = center + glm::vec3(radius * cos(nextRad), radius * sin(nextRad), 0.0f);
			DrawLine(p1, p2, color);
		}
	}

	void DebugRenderer::DrawFrustum(const glm::mat4& projection, const glm::vec4& color)
	{
		const glm::vec4 clipCorners[8] =
		{
			// Near
			{ -1.0f, -1.0f, 0.0f, 1.0f },
			{  1.0f, -1.0f, 0.0f, 1.0f },
			{  1.0f,  1.0f, 0.0f, 1.0f },
			{ -1.0f,  1.0f, 0.0f, 1.0f },

			// Far
			{ -1.0f, -1.0f, 1.0f, 1.0f },
			{  1.0f, -1.0f, 1.0f, 1.0f },
			{  1.0f,  1.0f, 1.0f, 1.0f },
			{ -1.0f,  1.0f, 1.0f, 1.0f }
		};

		glm::vec3 corners[8];
		const glm::mat4 invProjection = glm::inverse(projection);

		for (int i = 0; i < 8; ++i)
		{
			glm::vec4 corner = invProjection * clipCorners[i];
			corners[i] = glm::vec3(corner) / corner.w;
		}
		// Near plane.
		DrawLine(corners[0], corners[1], color);
		DrawLine(corners[1], corners[2], color);
		DrawLine(corners[2], corners[3], color);
		DrawLine(corners[3], corners[0], color);

		// Far plane.
		DrawLine(corners[4], corners[5], color);
		DrawLine(corners[5], corners[6], color);
		DrawLine(corners[6], corners[7], color);
		DrawLine(corners[7], corners[4], color);

		// Connect Near and Far.
		DrawLine(corners[0], corners[4], color);
		DrawLine(corners[1], corners[5], color);
		DrawLine(corners[2], corners[6], color);
		DrawLine(corners[3], corners[7], color);
	}
}