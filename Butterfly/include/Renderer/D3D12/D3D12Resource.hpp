#pragma once
#include "D3D12Common.hpp"
#include "Renderer/D3D12/D3D12SubresourceRange.hpp"

namespace Butterfly
{
	class D3D12CommandList;
	struct D3D12ResourceDescription;

	class D3D12Resource : private NonCopyable
	{
	public:
		D3D12Resource(const D3D12ResourceDescription& desc);
		~D3D12Resource();

		D3D12Resource* Write(const void* src, uint32_t numBytes, uint32_t offset = 0);
		uint32_t ArraySize() const;
		void* Map();
		void Unmap();
		D3D12Resource* TransitionAll(const D3D12CommandList& cmdList, const D3D12_RESOURCE_STATES& newState);
		D3D12Resource* TransitionSubresource(const D3D12CommandList& cmdList, const D3D12_RESOURCE_STATES& newState, uint32_t mipslice, uint32_t arraySlice);
		void TransitionRange(const D3D12CommandList& cmdList, const D3D12_RESOURCE_STATES& newState, const D3D12SubresourceRange& range);
		uint32_t NumSubresources() const;
		uint32_t MipCount() const;

		D3D12_RESOURCE_DESC BufferDescription() const { return m_bufferDescription; }
		D3D12_CLEAR_VALUE ClearValue() const { return m_clearValue; }
		CD3DX12_HEAP_PROPERTIES HeapProperties() const { return m_heapProperties; }

		ID3D12Resource2* HWResource() const { return m_hwResource; }

		friend class D3D12ResourceDescription;

	private:
		D3D12_CLEAR_VALUE m_clearValue;
		ID3D12Resource2* m_hwResource;
		D3D12_RESOURCE_DESC m_bufferDescription;
		CD3DX12_HEAP_PROPERTIES m_heapProperties;
		std::unordered_map<uint32_t, D3D12_RESOURCE_STATES> m_currentResourceStates;
		D3D12_RESOURCE_STATES m_wholeState;
		std::string m_debugName;
		bool m_isTransitionable;
	};

	struct D3D12ResourceDescription
	{
	public:
		static D3D12ResourceDescription Buffer(uint32_t numBytes);
		static D3D12ResourceDescription Texture(DXGI_FORMAT format, uint32_t width, uint32_t height, uint32_t arraySize = 1, uint32_t numMips = 1);
		static D3D12ResourceDescription Texture3D(DXGI_FORMAT format, uint32_t width, uint32_t height, uint32_t depth, uint32_t numMips = 1);
	
		D3D12ResourceDescription& FromSwapchain(ID3D12Resource2* resource, D3D12_RESOURCE_STATES state);
		D3D12ResourceDescription& SetName(const std::string& name);
		D3D12ResourceDescription& HeapType(D3D12_HEAP_TYPE type);
		D3D12ResourceDescription& ClearColor(float* color, DXGI_FORMAT format);
		D3D12ResourceDescription& ClearDepth(DXGI_FORMAT format);
		D3D12ResourceDescription& InitialState(const D3D12_RESOURCE_STATES& state);
		D3D12ResourceDescription& Flags(const D3D12_RESOURCE_FLAGS& flag);

		D3D12_CLEAR_VALUE ClearValue{};
		D3D12_RESOURCE_DESC BufferDescription{};
		CD3DX12_HEAP_PROPERTIES HeapProperties{};
		D3D12_RESOURCE_STATES WholeState{ D3D12_RESOURCE_STATE_COMMON };
		std::string DebugName;
		ID3D12Resource2* ResourceReference = nullptr;
	};
}
