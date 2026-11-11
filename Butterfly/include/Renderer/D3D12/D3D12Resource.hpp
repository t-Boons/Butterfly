#pragma once
#include "D3D12Common.hpp"

#define SUBRESOURCE_ZERO 0

namespace Butterfly
{
	class D3D12CommandList;

	class D3D12Resource : private NonCopyable
	{
	public:
		~D3D12Resource();

		D3D12Resource* Write(const void* src, uint32_t numBytes, uint32_t offset = 0);
		void* Map();
		void Unmap();
		D3D12Resource* Transition(const D3D12CommandList& cmdList, const D3D12_RESOURCE_STATES& newState);
		D3D12Resource* TransitionSubresource(const D3D12CommandList& cmdList, const D3D12_RESOURCE_STATES& newState, uint32_t mipslice, uint32_t arraySlice);
		D3D12Resource* UAVBarrier(const D3D12CommandList& cmdList);

		friend class DX12ResourceBuilder;

		D3D12_CLEAR_VALUE ClearValue;
		ID3D12Resource2* HwResource;
		D3D12_RESOURCE_DESC BufferDescription;
		CD3DX12_HEAP_PROPERTIES HeapProperties;
		std::unordered_map<uint32_t, D3D12_RESOURCE_STATES> CurrentResourceStates;
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
		DX12ResourceBuilder& Texture(DXGI_FORMAT format, uint32_t width, uint32_t height, uint32_t arraySize, uint32_t numMips = 1);
		DX12ResourceBuilder& Texture3D(DXGI_FORMAT format, uint32_t width, uint32_t height, uint32_t depth, uint32_t numMips = 1);
		DX12ResourceBuilder& RenderTarget(DXGI_FORMAT format, uint32_t width, uint32_t height);
		DX12ResourceBuilder& DepthStencil(DXGI_FORMAT format, uint32_t width, uint32_t height, uint32_t arraySize = 1);
		DX12ResourceBuilder& InitialState(const D3D12_RESOURCE_STATES& state);
		DX12ResourceBuilder& Flags(const D3D12_RESOURCE_FLAGS& flag);
		
	private:
		bool m_hasBeenCreated;
		D3D12Resource* m_resource;
	};
}
