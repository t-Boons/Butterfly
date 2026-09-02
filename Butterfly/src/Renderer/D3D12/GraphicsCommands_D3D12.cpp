#include "Renderer/D3D12/GraphicsCommands_DX12.hpp"
#include "Renderer/D3D12/Resource_DX12.hpp"
#include "Renderer/D3D12/View_DX12.hpp"
#include "Renderer/D3D12/GraphicsAPI_DX12.hpp"
#include "Renderer/D3D12/DescriptorAllocator_DX12.hpp"
#include "Renderer/D3D12/CommandList_DX12.hpp"
#include "Renderer/Texture_DX12.hpp"
#include "Renderer/Buffer_DX12.hpp"

namespace Butterfly
{
	void GraphicsCommands::SetRenderTargets(DX12CommandList& list, const std::vector<BFTexture*>& rts, BFTexture* dsv)
	{
		BF_PROFILE_EVENT();

		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvHandles;
		rtvHandles.reserve(8);
		for (uint32_t i = 0; i < rts.size(); i++)
		{
			rts[i]->Resource()->Transition(list, D3D12_RESOURCE_STATE_RENDER_TARGET);
			rtvHandles.push_back(rts[i]->RTV().Handle());
		}

		const D3D12_CPU_DESCRIPTOR_HANDLE* dsvHandle = nullptr;
		if (dsv)
		{
			dsv->Resource()->Transition(list, D3D12_RESOURCE_STATE_DEPTH_WRITE);
			dsvHandle = &dsv->DSV().Handle();
		}

		list.List()->OMSetRenderTargets(static_cast<uint32_t>(rts.size()), &rtvHandles[0], false, dsvHandle);
	}

	void GraphicsCommands::ClearRenderTarget(DX12CommandList& list, BFTexture& rt)
	{
		BF_PROFILE_EVENT();

		rt.Resource()->Transition(list, D3D12_RESOURCE_STATE_RENDER_TARGET);
		list.List()->ClearRenderTargetView(rt.RTV().Handle(), rt.ClearValue()->Color, 0, nullptr);
	}

	void GraphicsCommands::ClearRenderTarget(DX12CommandList& list, BFTexture& rt, const std::array<float, 4>& color)
	{
		BF_PROFILE_EVENT();

		rt.Resource()->Transition(list, D3D12_RESOURCE_STATE_RENDER_TARGET);
		list.List()->ClearRenderTargetView(rt.RTV().Handle(), &color.front(), 0, nullptr);
	}

	void GraphicsCommands::ClearDepthStencil(DX12CommandList& list, BFTexture& ds)
	{
		BF_PROFILE_EVENT();

		ds.Resource()->Transition(list, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		list.List()->ClearDepthStencilView(ds.DSV().Handle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	}

	void GraphicsCommands::SetFullscreenViewportAndRect(DX12CommandList& list, uint32_t width, uint32_t height)
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

	void GraphicsCommands::SetBindlessDescriptorHeapsAndRootSignature(DX12CommandList& list)
	{
		BF_PROFILE_EVENT();

		ID3D12DescriptorHeap* heaps[] = { DX12API()->DescriptorAllocatorSrvCbvUav()->Heap().Get(), DX12API()->DescriptorAllocatorSampler()->Heap().Get()};
		list.List()->SetDescriptorHeaps(_countof(heaps), heaps);
		list.List()->SetGraphicsRootSignature(DX12API()->BindlessRootSignature());
	}
};