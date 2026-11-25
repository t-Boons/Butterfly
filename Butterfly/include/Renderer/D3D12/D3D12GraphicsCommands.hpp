#pragma once
#include "D3D12Common.hpp"
#include "Math/AABB.hpp"

namespace Butterfly
{
	class D3D12CommandList;
	class BFTexture;
	class BFStructuredBuffer;
	class BFIndexBuffer;

	struct SDFTriangle
	{
		glm::vec3 Vertex0;
		glm::vec3 Vertex1;
		glm::vec3 Vertex2;
	};

	class GraphicsCommands
	{
	public:
		static RefPtr<BFTexture> CreateSDF(const std::vector<SDFTriangle>& triangleBuffer, const BFAABB& bounds, const glm::uvec3& resolution);
		static void Blit(D3D12CommandList& list, BFTexture& src, BFTexture& dst);
		static void GenerateMips(D3D12CommandList& list, BFTexture& texture, uint32_t face = 0);
		static void UploadTextureResource(BFTexture& dest, const void* data, bool generateMips = true);
		static void UploadTextureCubeResource(BFTexture& dest, const std::array<BFTexture*, 6>& cubemapFacesSource, bool generateMips = true);
		static void GenerateIrradiance(BFTexture& cubemapAtmosphere, BFTexture& irradiance, uint32_t size);

		static void EquirectangularToCubemap(D3D12CommandList& list, BFTexture& equirectangular, BFTexture& cubemap);
		static void SetFullscreenViewportAndRect(D3D12CommandList& list, uint32_t width, uint32_t height);
		static void SetBindlessDescriptorHeapsAndRootSignature(D3D12CommandList& list, bool isCompute);


	};
}
