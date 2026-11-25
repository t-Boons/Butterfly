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
	RefPtr<BFTexture> GraphicsCommands::CreateSDF(const std::vector<SDFTriangle>& triangleBuffer, const BFAABB& bounds, const glm::uvec3& resolution)
	{
		BF_PROFILE_EVENT()
			
		BFTextureDesc sdfDesc;
		sdfDesc.Width = resolution.x;
		sdfDesc.Height = resolution.y;
		sdfDesc.Depth = resolution.z;
		sdfDesc.Format = DXGI_FORMAT_R16_FLOAT;
		sdfDesc.ViewTypes = BFTextureDesc::ViewType::ShaderResource | BFTextureDesc::ViewType::UnorderedAccess;
		sdfDesc.DebugName = "SDF";
		sdfDesc.Dimention = BFTextureDimention::Texture3D;
		RefPtr<BFTexture> outSDF = MakeRef<BFTexture>(sdfDesc);

		BFStructuredBufferDesc triangleBufferDesc;
		triangleBufferDesc.Data = reinterpret_cast<void*>(const_cast<SDFTriangle*>(triangleBuffer.data()));
		triangleBufferDesc.NumElements = static_cast<uint32_t>(triangleBuffer.size());
		triangleBufferDesc.Stride = sizeof(SDFTriangle);
		triangleBufferDesc.DebugName = "SDF Triangles";
		triangleBufferDesc.HeapType = BFHeapType::Default;
		RefPtr<BFStructuredBuffer> triangleBufferGpu = MakeRef<BFStructuredBuffer>(triangleBufferDesc);

		struct SDFUniformData
		{
			glm::vec3 MinBounds;
			float Padding1;
			glm::vec3 MaxBounds;
			float Padding2;
			glm::uvec3 Resolution;
			uint32_t NumTriangles;
		};

		BFTypedConstantBuffer<SDFUniformData> sdfUniformBuffer;
		sdfUniformBuffer.Data.MinBounds = bounds.Min;
		sdfUniformBuffer.Data.MaxBounds = bounds.Max;
		sdfUniformBuffer.Data.Resolution = glm::uvec3(outSDF->Width(), outSDF->Height(), outSDF->Depth());
		sdfUniformBuffer.Data.NumTriangles = static_cast<uint32_t>(triangleBuffer.size());
		sdfUniformBuffer.Write();

		D3D12CommandList list(D3D12_COMMAND_LIST_TYPE_COMPUTE);

		list.StartComputePass("CreateSDF");

		BFComputePSOInfo pso;
		pso.ComputeShader = BFShaderCache::GetOrCreate("assets/Shaders/CreateSDF_cs.hlsl");

		BFShaderVariableBuilder(list)
			.Add(outSDF->UAV())
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

	void GraphicsCommands::Blit(D3D12CommandList& list, BFTexture& src, BFTexture& dst)
	{
		BF_PROFILE_EVENT()

		BF_CORE_ASSERT(src.Desc().Width == dst.Desc().Width && src.Desc().Height == dst.Desc().Height, "Source and destination textures must have the same dimensions for blitting: %s -> %s", src.Desc().DebugName.c_str(), dst.Desc().DebugName.c_str());
		BF_CORE_ASSERT(src.Desc().ViewTypes & BFTextureDesc::ViewType::ShaderResource, "Source texture must have the ShaderResource flag set: %s", src.Desc().DebugName.c_str());
		BF_CORE_ASSERT(dst.Desc().ViewTypes & BFTextureDesc::ViewType::RenderTargettable, "Destination texture must have the RenderTargettable flag set: %s", dst.Desc().DebugName.c_str());
		
		RasterPassStartInfo info;
		info.RenderTarget = &dst;
		info.RenderTargetLoadOp = LoadOP::Load;
		list.StartRenderPass(info, "Blit: " + src.Desc().DebugName + " -> " + dst.Desc().DebugName);

		list.SetViewport(RenderViewport::FromTexture(dst));
		BFGraphicsPSOInfo psoInfo;
		psoInfo.VertexShader = BFShaderCache::GetOrCreate("assets/Shaders/Fullscreen_vert.hlsl");
		psoInfo.PixelShader = BFShaderCache::GetOrCreate("assets/Shaders/Blit_frag.hlsl");
		psoInfo.DepthStencil.EnableDepth = false;
		psoInfo.Rasterizer.CullMode = D3D12_CULL_MODE_NONE;

		list.SetGraphicsPSO(psoInfo);

		BFShaderVariableBuilder(list)
			.Add(src.SRV())
			.Submit();


		list.DrawInstanced(6, 1, 0, 0);
		list.EndRenderPass();
	}

	void GraphicsCommands::GenerateMips(D3D12CommandList& list, BFTexture& texture, uint32_t face)
	{
		BF_PROFILE_EVENT()

		list.StartComputePass("Generate Mips: " + texture.Desc().DebugName);

		BFComputePSOInfo psoInfo;
		psoInfo.ComputeShader = BFShaderCache::GetOrCreate("assets/Shaders/Mip_cs.hlsl");

		list.SetComputePSO(psoInfo);

		const BFTextureDesc desc = texture.Desc();

		for (uint32_t mip = 1; mip < desc.NumMips; mip++)
		{
			const uint32_t width = std::max(1u, desc.Width >> mip);
			const uint32_t height = std::max(1u, desc.Height >> mip);

			BFShaderVariableBuilder(list)
				.Add(texture.SRV(BFSRVTextureViewInfo::Texture2DArray().Mips(mip - 1, 1).Slices(face, 1)))
				.Add(texture.UAV(BFUAVTextureViewInfo::Texture2DArray().Mip(mip).Slices(face, 1)))
				.Add(width)
				.Add(height)
				.Submit();

			list.Dispatch((width + 7) / 8, (height + 7) / 8, 1);
		}

		list.EndComputePass();
	}

	void GraphicsCommands::EquirectangularToCubemap(D3D12CommandList& list, BFTexture& equirectangular, BFTexture& cubemap)
	{
		BF_PROFILE_EVENT()

		BF_CORE_ASSERT(equirectangular.ViewTypes() & BFTextureDesc::ViewType::ShaderResource, "Source texture must have the ShaderResource flag set: %s", equirectangular.Desc().DebugName.c_str());
		BF_CORE_ASSERT(cubemap.ViewTypes() & BFTextureDesc::ViewType::UnorderedAccess, "Destination texture must have the RenderTargettable flag set: %s", cubemap.Desc().DebugName.c_str());
		BF_CORE_ASSERT(cubemap.Height() == cubemap.Width(), "Cubemap texture must be square: %s", cubemap.Desc().DebugName.c_str());
		BF_CORE_ASSERT(cubemap.Height() == equirectangular.Height(), "Cubemap texture must have the same height as the equirectangular texture: %s -> %s", equirectangular.Desc().DebugName.c_str(), cubemap.Desc().DebugName.c_str());

		list.StartComputePass("EquirectangularToCubemap");

		const uint32_t cubemapSize = cubemap.Desc().Height;
		BFComputePSOInfo pso;
		pso.ComputeShader = BFShaderCache::GetOrCreate("assets/Shaders/EquirectangularToCubemap_cs.hlsl");


		BFSampler& smp = BFSamplerCache::GetOrCreate(BFSamplerDesc::LinearWrap());
		BFShaderVariableBuilder(list)
			.Add(equirectangular.SRV())
			.Add(cubemap.UAV(BFUAVTextureViewInfo::Texture2DArray()))
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

	void GraphicsCommands::UploadTextureResource(BFTexture &dest, const void *data, bool generateMips)
	{
		BF_CORE_ASSERT(data, "If you want to copy data from CPU, you need to provide data");

		const D3D12_RESOURCE_DESC textureDesc = dest.Resource()->HWResource()->GetDesc();
		const uint32_t numSubresources = textureDesc.DepthOrArraySize;

		std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints(numSubresources);
		std::vector<UINT> numRows(numSubresources);
		std::vector<UINT64> rowSizes(numSubresources);

		uint64_t totalSize = 0;

		D3D12API()->Device()->GetCopyableFootprints(&textureDesc, 0, numSubresources, 0, footprints.data(), numRows.data(), rowSizes.data(), &totalSize);

		D3D12Resource *uploadResource = new D3D12Resource(
			D3D12ResourceDescription::Buffer(totalSize)
				.HeapType(D3D12_HEAP_TYPE_UPLOAD)
				.InitialState(D3D12_RESOURCE_STATE_COPY_SOURCE)
				.SetName("Intermediate texture."));

		const uint32_t sourceRowPitch = dest.Width() * GetBytesPerPixel(dest.Format());

		for (uint32_t slice = 0; slice < numSubresources; ++slice)
		{
			const uint8_t *src = static_cast<const uint8_t *>(data) + slice * dest.Height() * sourceRowPitch;
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

	void GraphicsCommands::UploadTextureCubeResource(BFTexture &dest, const std::array<BFTexture *, 6> &cubemapFacesSource, bool generateMips)
	{
		BF_PROFILE_EVENT();
		D3D12CommandList list;

		for (uint32_t face = 0; face < 6; ++face)
		{
			if (!cubemapFacesSource[face])
			{
				BF_CORE_LOG_WARN("Face %d of cubemap is null or has no resource. Skipping copy.", face);
				continue;
			}

			D3D12_TEXTURE_COPY_LOCATION src = {};
			src.pResource = cubemapFacesSource[face]->Resource()->HWResource();
			src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			src.SubresourceIndex = 0;

			D3D12_TEXTURE_COPY_LOCATION dst = {};
			dst.pResource = dest.Resource()->HWResource();
			dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			dst.SubresourceIndex = face;

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
