#pragma once
#include "D3D12Common.hpp"

namespace Butterfly
{
	class D3D12CommandList;

	class D3D12Resource : private NonCopyable
	{
	public:
		~D3D12Resource();

		D3D12Resource* Write(const void* src, uint32_t numBytes);
		D3D12Resource* Transition(const D3D12CommandList& cmdList, const D3D12_RESOURCE_STATES& newState);

		friend class DX12ResourceBuilder;

		D3D12_CLEAR_VALUE ClearValue;
		ID3D12Resource2* HwResource;
		D3D12_RESOURCE_DESC BufferDescription;
		CD3DX12_HEAP_PROPERTIES HeapProperties;
		D3D12_RESOURCE_STATES CurrentResourceState;
		std::string DebugName;

	private:
		D3D12Resource();

		void Alloc();
	};

	class DX12ResourceBuilder : private NonCopyable
	{
	public:
		DX12ResourceBuilder();
		~DX12ResourceBuilder();

		D3D12Resource* CreateFromSwapchain(ID3D12Resource2* resource, D3D12_RESOURCE_STATES state);
		D3D12Resource* Create();

		DX12ResourceBuilder& SetName(const std::string& name);
		DX12ResourceBuilder& HeapType(D3D12_HEAP_TYPE type);
		DX12ResourceBuilder& ClearColor(float* color, DXGI_FORMAT format);
		DX12ResourceBuilder& ClearDepth(DXGI_FORMAT format);
		DX12ResourceBuilder& Buffer(const size_t& numBytes);
		DX12ResourceBuilder& IndexBuffer(const size_t& numBytes);
		DX12ResourceBuilder& Texture2D(DXGI_FORMAT format, uint32_t width, uint32_t height);
		DX12ResourceBuilder& RenderTarget(DXGI_FORMAT format, uint32_t width, uint32_t height);
		DX12ResourceBuilder& DepthStencil(DXGI_FORMAT format, uint32_t width, uint32_t height);
		DX12ResourceBuilder& InitialState(const D3D12_RESOURCE_STATES& state);

	private:
		bool m_hasBeenCreated;
		D3D12Resource* m_resource;
	};
}
