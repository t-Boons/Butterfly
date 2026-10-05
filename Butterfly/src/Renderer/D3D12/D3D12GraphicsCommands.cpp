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

namespace Butterfly
{
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

	void GraphicsCommands::SetBindlessDescriptorHeapsAndRootSignature(D3D12CommandList& list)
	{
		BF_PROFILE_EVENT();

		ID3D12DescriptorHeap* heaps[] = { D3D12API()->DescriptorAllocatorSrvCbvUav()->Heap().Get(), D3D12API()->DescriptorAllocatorSampler()->Heap().Get() };
		list.List()->SetDescriptorHeaps(_countof(heaps), heaps);
		list.List()->SetGraphicsRootSignature(D3D12API()->BindlessRootSignature());
		list.List()->SetComputeRootSignature(D3D12API()->BindlessRootSignature());
	}
};
