#pragma once
#include "Common_DX12.hpp"

namespace Butterfly
{
	class DX12CommandList;

	class DX12Resource : private NonCopyable
	{
	public:
		~DX12Resource();

		DX12Resource* Write(const void* src, uint32_t numBytes);
		DX12Resource* Transition(const DX12CommandList& cmdList, const D3D12_RESOURCE_STATES& newState);

		friend class DX12ResourceBuilder;

		D3D12_CLEAR_VALUE ClearValue;
		ID3D12Resource2* HwResource;
		D3D12_RESOURCE_DESC BufferDescription;
		CD3DX12_HEAP_PROPERTIES HeapProperties;
		D3D12_RESOURCE_STATES CurrentResourceState;
		std::string DebugName;

	private:
		DX12Resource();

		void Alloc();
	};

	class DX12ResourceBuilder : private NonCopyable
	{
	public:
		DX12ResourceBuilder();
		~DX12ResourceBuilder();

		DX12Resource* CreateFromSwapchain(ID3D12Resource2* resource, D3D12_RESOURCE_STATES state);
		DX12Resource* Create();

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
		DX12Resource* m_resource;
	};
}