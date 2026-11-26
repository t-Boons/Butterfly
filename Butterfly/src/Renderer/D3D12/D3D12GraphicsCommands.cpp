#include "Renderer/D3D12/D3D12GraphicsCommands.hpp"
#include "Renderer/D3D12/D3D12Resource.hpp"
#include "Renderer/D3D12/D3D12View.hpp"
#include "Renderer/D3D12/D3D12GraphicsAPI.hpp"
#include "Renderer/D3D12/D3D12DescriptorAllocator.hpp"
#include "Renderer/D3D12/D3D12CommandList.hpp"
#include "Renderer/D3D12Texture.hpp"
#include "Renderer/D3D12Buffer.hpp"
#include "Renderer/D3D12/D3D12Pipeline.hpp"
#include "Renderer/D3D12/D3D12Shader.hpp"
#include "Renderer/D3D12/D3D12ShaderVariables.hpp"
#include "Renderer/D3D12Sampler.hpp"
#include "Renderer/D3D12/D3D12CommandQueue.hpp"

namespace Butterfly
{
	RefPtr<Texture> GraphicsCommands::CreateSDF(const std::vector<SDFTriangle>& triangleBuffer, const AABB& bounds, const glm::uvec3& resolution)
	{
		BF_PROFILE_EVENT()
			
		TextureDesc sdfDesc;
		sdfDesc.Width = resolution.x;
		sdfDesc.Height = resolution.y;
		sdfDesc.Depth = resolution.z;
		sdfDesc.Format = DXGI_FORMAT_R16_FLOAT;
		sdfDesc.ViewTypes = TextureDesc::ViewType::ShaderResource | TextureDesc::ViewType::UnorderedAccess;
		sdfDesc.DebugName = "SDF";
		sdfDesc.Dimension = TextureDimension::Texture3D;
		RefPtr<Texture> outSDF = MakeRef<Texture>(sdfDesc);

		StructuredBufferDesc triangleBufferDesc;
		triangleBufferDesc.Data = reinterpret_cast<void*>(const_cast<SDFTriangle*>(triangleBuffer.data()));
		triangleBufferDesc.NumElements = static_cast<uint32_t>(triangleBuffer.size());
		triangleBufferDesc.Stride = sizeof(SDFTriangle);
		triangleBufferDesc.DebugName = "SDF Triangles";
		triangleBufferDesc.HeapType = HeapType::Default;
		RefPtr<StructuredBuffer> triangleBufferGpu = MakeRef<StructuredBuffer>(triangleBufferDesc);

		struct SDFUniformData
		{
			glm::vec3 MinBounds;
			float Padding1;
			glm::vec3 MaxBounds;
			float Padding2;
			glm::uvec3 Resolution;
			uint32_t NumTriangles;
		};

		TypedConstantBuffer<SDFUniformData> sdfUniformBuffer;
		sdfUniformBuffer.Data.MinBounds = bounds.Min;
		sdfUniformBuffer.Data.MaxBounds = bounds.Max;
		sdfUniformBuffer.Data.Resolution = glm::uvec3(outSDF->Width(), outSDF->Height(), outSDF->Depth());
		sdfUniformBuffer.Data.NumTriangles = static_cast<uint32_t>(triangleBuffer.size());
		sdfUniformBuffer.Write();

		D3D12CommandList list(D3D12_COMMAND_LIST_TYPE_COMPUTE);

		list.StartComputePass("CreateSDF");

		ComputePSOInfo pso;
		pso.ComputeShader = ShaderCache::GetOrCreate("CreateSDF_cs.hlsl");

		ShaderVariableBuilder(list)
			.Add(outSDF->UAV(UAVTextureViewInfo::Texture3D()))
			.Add(triangleBufferGpu->SRV())
			.Add(sdfUniformBuffer.CBV())
			.Submit();

		list.SetComputePSO(pso);

		list.Dispatch((outSDF->Width() + 7) / 8, (outSDF->Height() + 7) / 8, (outSDF->Depth() + 7) / 8);

		list.EndComputePass();

		list.Close();
		D3D12API()->Queue(QueueType::Compute)->Execute(list);
		D3D12API()->Queue(QueueType::Compute)->WaitForFence();

		return outSDF;
	}

	void GraphicsCommands::Blit(D3D12CommandList& list, Texture& src, Texture& dst)
	{
		BF_PROFILE_EVENT()

		BF_CORE_ASSERT(src.Width() == dst.Width() && src.Height() == dst.Height(), "Source and destination textures must have the same dimensions for blitting: %s -> %s", src.DebugName().c_str(), dst.DebugName().c_str());
		BF_CORE_ASSERT(src.ViewTypes() & TextureDesc::ViewType::ShaderResource, "Source texture must have the ShaderResource flag set: %s", src.DebugName().c_str());
		BF_CORE_ASSERT(dst.ViewTypes() & TextureDesc::ViewType::RenderTargetable, "Destination texture must have the RenderTargetable flag set: %s", dst.DebugName().c_str());
		
		RasterPassStartInfo info;
		info.RenderTarget = &dst;
		info.RenderTargetLoadOp = LoadOP::Load;
		list.StartRenderPass(info, "Blit: " + src.DebugName() + " -> " + dst.DebugName());

		list.SetViewport(RenderViewport::FromTexture(dst));
		GraphicsPSOInfo psoInfo;
		psoInfo.VertexShader = ShaderCache::GetOrCreate("Fullscreen_vert.hlsl");
		psoInfo.PixelShader = ShaderCache::GetOrCreate("Blit_frag.hlsl");
		psoInfo.DepthStencil.EnableDepth = false;
		psoInfo.Rasterizer.CullMode = D3D12_CULL_MODE_NONE;

		list.SetGraphicsPSO(psoInfo);

		ShaderVariableBuilder(list)
			.Add(src.SRV(SRVTextureViewInfo::Texture2D()))
			.Submit();


		list.DrawInstanced(6, 1, 0, 0);
		list.EndRenderPass();
	}

	void GraphicsCommands::GenerateMips(D3D12CommandList& list, Texture& texture, uint32_t face)
	{
		BF_PROFILE_EVENT()

		list.StartComputePass("Generate Mips: " + texture.DebugName());

		ComputePSOInfo psoInfo;
		psoInfo.ComputeShader = ShaderCache::GetOrCreate("Mip_cs.hlsl");

		list.SetComputePSO(psoInfo);

		for (uint32_t mip = 1; mip < texture.NumMips(); mip++)
		{
			const uint32_t width = std::max(1u, texture.Width() >> mip);
			const uint32_t height = std::max(1u, texture.Height() >> mip);

			ShaderVariableBuilder(list)
				.Add(texture.SRV(SRVTextureViewInfo::Texture2DArray().Mips(mip - 1, 1).Slices(face, 1)))
				.Add(texture.UAV(UAVTextureViewInfo::Texture2DArray().Mip(mip).Slices(face, 1)))
				.Add(width)
				.Add(height)
				.Submit();

			list.Dispatch((width + 7) / 8, (height + 7) / 8, 1);
		}

		list.EndComputePass();
	}

	void GraphicsCommands::EquirectangularToCubemap(D3D12CommandList& list, Texture& equirectangular, Texture& cubemap)
	{
		BF_PROFILE_EVENT()

		BF_CORE_ASSERT(equirectangular.ViewTypes() & TextureDesc::ViewType::ShaderResource, "Source texture must have the ShaderResource flag set: %s", equirectangular.DebugName().c_str());
		BF_CORE_ASSERT(cubemap.ViewTypes() & TextureDesc::ViewType::UnorderedAccess, "Destination texture must have the RenderTargetable flag set: %s", cubemap.DebugName().c_str());
		BF_CORE_ASSERT(cubemap.Height() == cubemap.Width(), "Cubemap texture must be square: %s", cubemap.DebugName().c_str());
		BF_CORE_ASSERT(cubemap.Height() == equirectangular.Height(), "Cubemap texture must have the same height as the equirectangular texture: %s -> %s", equirectangular.DebugName().c_str(), cubemap.DebugName().c_str());

		list.StartComputePass("EquirectangularToCubemap");

		const uint32_t cubemapSize = cubemap.Height();
		ComputePSOInfo pso;
		pso.ComputeShader = ShaderCache::GetOrCreate("EquirectangularToCubemap_cs.hlsl");


		Sampler& smp = SamplerCache::GetOrCreate(SamplerDesc::LinearWrap());
		ShaderVariableBuilder(list)
			.Add(equirectangular.SRV(SRVTextureViewInfo::Texture2D()))
			.Add(cubemap.UAV(UAVTextureViewInfo::Texture2DArray()))
			.Add(smp)
			.Add(cubemap.Height())
			.Submit();

		list.SetComputePSO(pso);
		const uint32_t groupsX = (cubemapSize + 7) / 8;
		const uint32_t groupsY = (cubemapSize + 7) / 8;
		list.Dispatch(groupsX, groupsY, 6);

		list.EndComputePass();
	}

	void GraphicsCommands::SetFullscreenViewportAndRect(D3D12CommandList& list, uint32_t width, uint32_t height)
	{
		BF_PROFILE_EVENT();

		D3D12_VIEWPORT vp;
		vp.TopLeftX = 0.0f;
		vp.TopLeftY = 0.0f;
		vp.Width = static_cast<FLOAT>(width);
		vp.Height = static_cast<FLOAT>(height);
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;

		D3D12_RECT scissorRect = { 0u, 0u, static_cast<LONG>(width), static_cast<LONG>(height) };
		list.List()->RSSetScissorRects(1, &scissorRect);
		list.List()->RSSetViewports(1, &vp);
	}

	void GraphicsCommands::UploadTextureResource(Texture &dest, const void* data, uint32_t dataSize, bool generateMips)
	{
		BF_PROFILE_EVENT();

		BF_CORE_ASSERT(data != nullptr, "If you want to copy data from CPU, you need to provide data");

		const D3D12_RESOURCE_DESC textureDesc = dest.Resource()->HWResource()->GetDesc();
		const uint32_t numSubresources = textureDesc.DepthOrArraySize;

		std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints(numSubresources);
		std::vector<UINT> numRows(numSubresources);
		std::vector<UINT64> rowSizes(numSubresources);

		uint64_t totalSize = 0;

		D3D12API()->Device()->GetCopyableFootprints(&textureDesc, 0, numSubresources, 0, footprints.data(), numRows.data(), rowSizes.data(), &totalSize);

		BF_CORE_ASSERT(dataSize == totalSize, "Provided data is different than the required texture size");

		D3D12Resource *uploadResource = new D3D12Resource(
			D3D12ResourceDescription::Buffer(totalSize)
				.HeapType(D3D12_HEAP_TYPE_UPLOAD)
				.InitialState(D3D12_RESOURCE_STATE_COPY_SOURCE)
				.SetName("Intermediate texture."));

		const uint32_t sourceRowPitch = dest.Width() * GetBytesPerPixel(dest.Format());

		for (uint32_t slice = 0; slice < numSubresources; ++slice)
		{
			const uint8_t *src = static_cast<const uint8_t*>(data) + slice * dest.Height() * sourceRowPitch;
			const auto &footprint = footprints[slice];

			const uint32_t dstRowPitch = footprint.Footprint.RowPitch;
			const uint32_t copyRowSize = static_cast<uint32_t>(rowSizes[slice]);

			for (uint32_t y = 0; y < dest.Height(); ++y)
			{
				uploadResource->Write(src + y * sourceRowPitch, copyRowSize, static_cast<uint32_t>(footprint.Offset + y * dstRowPitch));
			}
		}

		D3D12CommandList list;

		for (uint32_t slice = 0; slice < numSubresources; ++slice)
		{
			D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
			srcLocation.pResource = uploadResource->HWResource();
			srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			srcLocation.PlacedFootprint = footprints[slice];

			D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
			dstLocation.pResource = dest.Resource()->HWResource();
			dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			dstLocation.SubresourceIndex = slice;

			dest.Resource()->TransitionAll(list, D3D12_RESOURCE_STATE_COPY_DEST);

			list.List()->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
		}

		if (dest.NumMips() > 1 && generateMips)
		{
			GraphicsCommands::GenerateMips(list, dest);
		}

		dest.Resource()->TransitionAll(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		list.Close();

		D3D12API()->Queue(QueueType::Direct)->Execute(list);
		D3D12API()->Queue(QueueType::Direct)->WaitForFence();

		delete uploadResource;
	}

	void GraphicsCommands::UploadTextureCubeResource(Texture &dest, const std::array<Texture *, 6> &cubemapFacesSource, bool generateMips)
	{
		BF_PROFILE_EVENT();

		BF_CORE_ASSERT(dest.ArraySize() == 6, "Destination cubemap must have 6 faces: %s", dest.DebugName().c_str());

		D3D12CommandList list;


		dest.Resource()->TransitionAll(list, D3D12_RESOURCE_STATE_COPY_DEST);

		for (uint32_t face = 0; face < 6; ++face)
		{
			if (!cubemapFacesSource[face])
			{
				BF_CORE_LOG_WARN("Face %d of cubemap is null or has no resource. Skipping copy.", face);
				continue;
			}

			D3D12SubresourceRange range;
			range.SliceRange(0, 1).MipRange(0, 1);
			cubemapFacesSource[face]->Resource()->TransitionRange(list, D3D12_RESOURCE_STATE_COPY_SOURCE, range);

			D3D12_TEXTURE_COPY_LOCATION src = {};
			src.pResource = cubemapFacesSource[face]->Resource()->HWResource();
			src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			src.SubresourceIndex = 0;

			D3D12_TEXTURE_COPY_LOCATION dst = {};
			dst.pResource = dest.Resource()->HWResource();
			dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			dst.SubresourceIndex = D3D12CalcSubresource(0, face, 0, dest.NumMips(), 6);

			list.List()->CopyTextureRegion(
				&dst,
				0, 0, 0,
				&src,
				nullptr);
		}

		for (uint32_t i = 0; i < 6; ++i)
		{
			GraphicsCommands::GenerateMips(list, dest, i);
		}

		dest.Resource()->TransitionAll(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		list.Close();

		D3D12API()->Queue(QueueType::Direct)->Execute(list);
		D3D12API()->Queue(QueueType::Direct)->WaitForFence();
	}

	void GraphicsCommands::GenerateIrradiance(Texture& cubemapAtmosphere, Texture& irradiance, uint32_t size)
	{
		BF_PROFILE_EVENT();

		BF_CORE_ASSERT(size == irradiance.Width(), "Irradiance size must be equal to the specified size: %d", size);
		BF_CORE_ASSERT(size == irradiance.Height(), "Irradiance size must be equal to the specified size: %d", size);
		BF_CORE_ASSERT(cubemapAtmosphere.ArraySize() == irradiance.ArraySize(), "Source and destination textures must have the same dimensions: %s -> %s", cubemapAtmosphere.DebugName().c_str(), irradiance.DebugName().c_str());
		BF_CORE_ASSERT(cubemapAtmosphere.ViewTypes() & TextureDesc::ViewType::ShaderResource, "Source texture must have the ShaderResource flag set: %s", cubemapAtmosphere.DebugName().c_str());
		BF_CORE_ASSERT(irradiance.ViewTypes() & TextureDesc::ViewType::UnorderedAccess, "Destination texture must have the UnorderedAccess flag set: %s", irradiance.DebugName().c_str());
		
		D3D12CommandList list;

		list.StartComputePass("GenerateIrradiance: " + cubemapAtmosphere.DebugName() + " -> " + irradiance.DebugName());

		ComputePSOInfo psoInfo;
		psoInfo.ComputeShader = ShaderCache::GetOrCreate("Irradiance_cs.hlsl");

		list.SetComputePSO(psoInfo);

		ShaderVariableBuilder(list)
			.Add(cubemapAtmosphere.SRV(SRVTextureViewInfo::Cubemap()))
			.Add(irradiance.UAV(UAVTextureViewInfo::Texture2DArray()))
			.Add(SamplerCache::GetOrCreate(SamplerDesc::LinearClamp()))
			.Submit();

		list.Dispatch((size + 7) / 8, (size + 7) / 8, 6);

		irradiance.Resource()->TransitionAll(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		list.EndComputePass();

		list.Close();

		D3D12API()->Queue(QueueType::Direct)->Execute(list);
		D3D12API()->Queue(QueueType::Direct)->WaitForFence();
	}

	void GraphicsCommands::SetBindlessDescriptorHeapsAndRootSignature(D3D12CommandList& list, bool isCompute)
	{
		BF_PROFILE_EVENT();

		ID3D12DescriptorHeap* heaps[] = { D3D12API()->DescriptorAllocatorSrvCbvUav()->Heap().Get(), D3D12API()->DescriptorAllocatorSampler()->Heap().Get() };
		list.List()->SetDescriptorHeaps(_countof(heaps), heaps);

		if (isCompute)
		{
			list.List()->SetComputeRootSignature(D3D12API()->BindlessRootSignature());
		}
		else
		{
			list.List()->SetGraphicsRootSignature(D3D12API()->BindlessRootSignature());
		}
	}
};
