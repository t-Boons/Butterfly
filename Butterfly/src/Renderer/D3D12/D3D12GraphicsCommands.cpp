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
	RefPtr<BFTexture> GraphicsCommands::CreateSDF(const std::vector<SDFTriangle>& triangleBuffer, const Bounds& bounds)
	{
		BF_PROFILE_EVENT()
			
		BFTextureDesc sdfDesc;
		sdfDesc.Width = 128;
		sdfDesc.Height = 128;
		sdfDesc.Depth = 128;
		sdfDesc.Format = DXGI_FORMAT_R32_FLOAT;
		sdfDesc.ViewTypes = BFTextureDesc::ViewType::ShaderResource | BFTextureDesc::ViewType::UnorderedAccess;
		sdfDesc.DebugName = "SDF";
		sdfDesc.Type = BFTextureType::Texture3D;
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
		} data;
		
		data.MinBounds = bounds.Min;
		data.MaxBounds = bounds.Max;
		data.Resolution = glm::uvec3(outSDF->Width(), outSDF->Height(), outSDF->Depth());
		data.NumTriangles = static_cast<uint32_t>(triangleBuffer.size());

		BFUniformBuffer uniformBufferDesc(sizeof(SDFUniformData), "SDFUniforms");
		uniformBufferDesc.GetOrCreateView(sizeof(SDFUniformData), HASH("SDFUniforms"));
		uniformBufferDesc.Write(&data, sizeof(SDFUniformData), HASH("SDFUniforms"));

		D3D12CommandList list(D3D12_COMMAND_LIST_TYPE_COMPUTE);

		list.StartComputePass("CreateSDF");

		outSDF->Resource()->Transition(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		BFComputePSOInfo pso;
		pso.ComputeShader = BFShaderCache::GetOrCreate(L"assets/Shaders/CreateSDF_cs.hlsl", ShaderType::Compute);

		ShaderVariables()
			.Add(outSDF->UAV().View())
			.Add(triangleBufferGpu->SRV().View())
			.Add(uniformBufferDesc.GetView(HASH("SDFUniforms"))->View())
			.Submit(list, true);

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
		psoInfo.VertexShader = BFShaderCache::GetOrCreate(L"assets/Shaders/Fullscreen_vert.hlsl", ShaderType::Vertex);
		psoInfo.PixelShader = BFShaderCache::GetOrCreate(L"assets/Shaders/Blit_frag.hlsl", ShaderType::Pixel);
		psoInfo.DepthStencil.EnableDepth = false;
		psoInfo.Rasterizer.CullMode = D3D12_CULL_MODE_NONE;

		list.SetGraphicsPSO(psoInfo);

		src.Resource()->Transition(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		ShaderVariables()
			.Add(src.SRV().View())
			.Submit(list);


		list.DrawInstanced(6, 1, 0, 0);
		list.EndRenderPass();
	}

	void GraphicsCommands::EquirectangularToCubemap(D3D12CommandList& list, BFTexture& equirectangular, BFTexture& cubemap)
	{
		BF_PROFILE_EVENT()

		BF_CORE_ASSERT(equirectangular.ViewTypes() & BFTextureDesc::ViewType::ShaderResource, "Source texture must have the ShaderResource flag set: %s", equirectangular.Desc().DebugName.c_str());
		BF_CORE_ASSERT(cubemap.ViewTypes() & BFTextureDesc::ViewType::UnorderedAccess, "Destination texture must have the RenderTargettable flag set: %s", cubemap.Desc().DebugName.c_str());
		BF_CORE_ASSERT(cubemap.Height() == cubemap.Width(), "Cubemap texture must be square: %s", cubemap.Desc().DebugName.c_str());
		BF_CORE_ASSERT(cubemap.Height() == equirectangular.Height(), "Cubemap texture must have the same height as the equirectangular texture: %s -> %s", equirectangular.Desc().DebugName.c_str(), cubemap.Desc().DebugName.c_str());
		BF_CORE_ASSERT(cubemap.Type() == BFTextureType::Cubemap, "Destination texture must be a cubemap: %s", cubemap.Desc().DebugName.c_str());

		list.StartComputePass("EquirectangularToCubemap");

		cubemap.Resource()->Transition(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		const uint32_t cubemapSize = cubemap.Desc().Height;
		BFComputePSOInfo pso;
		pso.ComputeShader = BFShaderCache::GetOrCreate(L"assets/Shaders/EquirectangularToCubemap_cs.hlsl", ShaderType::Compute);


		BFSampler smp;
		ShaderVariables()
			.Add(equirectangular.SRV().View())
			.Add(cubemap.UAV().View())
			.Add(smp.View())
			.Add(cubemap.Height())
			.Submit(list, true);

		list.SetComputePSO(pso);
		const uint32_t groupsX = (cubemapSize + 7) / 8;
		const uint32_t groupsY = (cubemapSize + 7) / 8;
		list.Dispatch(groupsX, groupsY, 6);

		cubemap.Resource()->Transition(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

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
