#pragma once
#include "D3D12Common.hpp"
#include "Math/AABB.hpp"

namespace Butterfly
{
	class D3D12CommandList;
	class Texture;
	class StructuredBuffer;
	class IndexBuffer;

	struct SDFTriangle
	{
		glm::vec3 Vertex0;
		glm::vec3 Vertex1;
		glm::vec3 Vertex2;
	};

	class GraphicsCommands
	{
	public:
		static RefPtr<Texture> CreateSDF(const std::vector<SDFTriangle>& triangleBuffer, const AABB& bounds, const glm::uvec3& resolution);
		static void Blit(D3D12CommandList& list, Texture& src, Texture& dst);
		static void GenerateMips(D3D12CommandList& list, Texture& texture, uint32_t face = 0);

		static void UploadTextureResource(Texture& dest, const void* data, uint32_t dataSize, bool generateMips = true);
		static void UploadTextureResource(Texture& dest, const std::vector<uint8_t>& data, bool generateMips = true)
		{
			UploadTextureResource(dest, data.data(), static_cast<uint32_t>(data.size()), generateMips);
		}
		
		static void UploadTextureCubeResource(Texture& dest, const std::array<Texture*, 6>& cubemapFacesSource, bool generateMips = true);
		static void GenerateIrradiance(Texture& cubemapAtmosphere, Texture& irradiance, uint32_t size);
		static void GeneratePrefilter(Texture& cubemapAtmosphere, Texture& prefilter);

		static void EquirectangularToCubemap(D3D12CommandList& list, Texture& equirectangular, Texture& cubemap);
		static void SetFullscreenViewportAndRect(D3D12CommandList& list, uint32_t width, uint32_t height);
		static void SetBindlessDescriptorHeapsAndRootSignature(D3D12CommandList& list, bool isCompute);


	};
}
