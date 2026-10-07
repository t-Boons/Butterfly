#pragma once
#include "D3D12Common.hpp"

namespace Butterfly
{
	class D3D12CommandList;
	class BFTexture;
	class BFStructuredBuffer;
	class BFIndexBuffer;

	struct Bounds
	{
		glm::vec3 Size() const { return Max - Min; }
		glm::vec3 Min = glm::vec3{ 0.0 };
		glm::vec3 Max = glm::vec3{ 0.0 };
	};

	struct SDFTriangle
	{
		glm::vec3 Vertex0;
		glm::vec3 Vertex1;
		glm::vec3 Vertex2;
	};

	class GraphicsCommands
	{
	public:
		static RefPtr<BFTexture> CreateSDF(const std::vector<SDFTriangle>& triangleBuffer, const Bounds& bounds, const glm::uvec3& resolution);
		static void Blit(D3D12CommandList& list, BFTexture& src, BFTexture& dst);
		static void EquirectangularToCubemap(D3D12CommandList& list, BFTexture& equirectangular, BFTexture& cubemap);
		static void SetFullscreenViewportAndRect(D3D12CommandList& list, uint32_t width, uint32_t height);
		static void SetBindlessDescriptorHeapsAndRootSignature(D3D12CommandList& list, bool isCompute);
	};
}
