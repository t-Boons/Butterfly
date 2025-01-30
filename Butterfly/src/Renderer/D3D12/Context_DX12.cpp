#include "Renderer/D3D12/Context_DX12.hpp"
#include "Renderer/D3D12/CommandQueue_DX12.hpp"
#include "Renderer/D3D12/CommandList_DX12.hpp"
#include "Renderer/D3D12/Resource_DX12.hpp"
#include "Renderer/D3D12/GraphicsAPI_DX12.hpp"
#include "Renderer/D3D12/Fence_DX12.hpp"

namespace Butterfly
{
	BFGraphicsContext::~BFGraphicsContext()
	{
		FreeContext();
	}

	BFGraphicsContext::BFGraphicsContext(HWND hwnd)
		: m_hwnd(hwnd), m_ctxInitialized(false), VSync(false)
	{
		BF_PROFILE_EVENT()

		RECT rect;
		if (!GetClientRect(hwnd, &rect))
		{
			Throw("Can not get width and height from Hwnd");
		}

		Resize(rect.right - rect.left, rect.bottom - rect.top);
		m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
	}

	void BFGraphicsContext::Resize(uint32_t width, uint32_t height)
	{
		BF_PROFILE_EVENT()

		FreeContext();

		m_width = width;
		m_height = height;

		CreateContext();

		Log::Info("Resized GraphicsContext window to: %u x %u", width, height);
	}

	void BFGraphicsContext::CreateContext()
	{
		if (m_ctxInitialized)
		{
			Throw("BFGraphicsContext is already initialized");
			return;
		}

		m_ctxInitialized = true;

		m_fence = new DX12Fence();

		for (uint32_t i = 0; i < BUFFER_COUNT; i++)
		{
			m_swapchainCmdList[i] = new DX12CommandList();
		}


		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.Width = m_width;                      
		swapChainDesc.Height = m_height;                    
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.Stereo = FALSE;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = BUFFER_COUNT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

		IDXGISwapChain1* tempSwapChain;
		ThrowIfFailed(DX12API()->Factory()->CreateSwapChainForHwnd(DX12API()->Queue(QueueType::Direct)->D3D12Queue(), m_hwnd, &swapChainDesc, nullptr, nullptr, &tempSwapChain));
		m_swapChain = reinterpret_cast<IDXGISwapChain4*>(tempSwapChain);

		for (uint32_t i = 0; i < BUFFER_COUNT; ++i)
		{
			ID3D12Resource2* renderTarget;
			ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTarget)));

			m_renderTargets[i] = DX12ResourceBuilder()
				.RenderTarget(DXGI_FORMAT_R8G8B8A8_UNORM, m_width, m_height)
				.SetName(("BackBuffer: " + std::to_string(i)).c_str())
				.CreateFromSwapchain(renderTarget, D3D12_RESOURCE_STATE_PRESENT);
		}
		m_backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
	}

	void BFGraphicsContext::FreeContext()
	{
		if (m_ctxInitialized)
		{
			m_ctxInitialized = false;

			FREE(m_fence);

			// Safe release because its a DX12 type;
			COM_FREE(m_swapChain);
			
			for (uint32_t i = 0; i < BUFFER_COUNT; i++)
			{
				FREE(m_renderTargets[i]);
				FREE(m_swapchainCmdList[i]);
			}
		}
	}

	void BFGraphicsContext::DisplayTexture(DX12Resource& toDisplay)
	{
		BF_PROFILE_EVENT()

		m_frameIndex++;

		m_fence->Wait();

		m_backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

		DX12Resource& backBuffer = *m_renderTargets[m_backBufferIndex];
		DX12CommandList& list = *m_swapchainCmdList[m_backBufferIndex];
		list.Reset();

		// Record copy.
		{
			list.BeginGPUMarker("Copy to backbuffer index: " + std::to_string(m_backBufferIndex));

			backBuffer.Transition(list, D3D12_RESOURCE_STATE_COPY_DEST);
			toDisplay.Transition(list, D3D12_RESOURCE_STATE_COPY_SOURCE);

			D3D12_BOX srcBox = {};
			srcBox.left = 0;
			srcBox.top = 0;
			srcBox.front = 0;
			srcBox.right = m_width;
			srcBox.bottom = m_height;
			srcBox.back = 1;

			D3D12_TEXTURE_COPY_LOCATION destLocation = {};
			destLocation.pResource = backBuffer.HwResource;
			destLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			destLocation.SubresourceIndex = 0;

			D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
			srcLocation.pResource = toDisplay.HwResource;
			srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			srcLocation.SubresourceIndex = 0;


			list.List()->CopyTextureRegion(
				&destLocation, 
				0, 0, 0,       
				&srcLocation,  
				&srcBox        
			);

			backBuffer.Transition(list, D3D12_RESOURCE_STATE_PRESENT);
			list.EndGPUMarker();
			list.Close();
		}

		DX12API()->Queue(QueueType::Direct)->Execute(list);


		uint32_t flags = DXGI_PRESENT_ALLOW_TEARING;
		uint32_t syncInterval = 0;

		if (VSync)
		{
			flags = 0;
			syncInterval = 1;
		}

		ThrowIfFailed(m_swapChain->Present(syncInterval, flags));
		m_fence->Signal(*DX12API()->Queue(QueueType::Direct));
	}
}