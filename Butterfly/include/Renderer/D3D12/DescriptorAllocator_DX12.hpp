#pragma once
#include "Common_DX12.hpp"

namespace Butterfly
{
	class DX12Resource;

	class DX12DescriptorAllocator : private NonCopyable
	{
	public:
		void FreeHandle(uint32_t handle);
		const ComPtr<ID3D12DescriptorHeap> Heap() const { return m_heap; }

	protected:
		DX12DescriptorAllocator(const D3D12_DESCRIPTOR_HEAP_DESC description, const std::wstring& resourceTag);
		uint32_t NextFreeHandle();

		const D3D12_DESCRIPTOR_HEAP_DESC m_desc;
		const uint32_t m_incrementSize;
		ComPtr<ID3D12DescriptorHeap> m_heap;

		uint32_t m_nextFreeIndex;
		std::vector<uint32_t> m_freedDiscriptors;
	};


	class DX12DescriptorAllocatorCbvSrvUav : public DX12DescriptorAllocator
	{
	public:
		DX12DescriptorAllocatorCbvSrvUav(const uint32_t numDescriptors);

		uint32_t CreateCbv(uint32_t numBytes, const DX12Resource& resource);
		uint32_t CreateSrv(const DX12Resource& resource, const D3D12_SHADER_RESOURCE_VIEW_DESC* description);
	
		D3D12_GPU_DESCRIPTOR_HANDLE  GpuHandleFromSrvHandle(uint32_t handle) const;
		void AllocateDummy();
	};

	class DX12DescriptorAllocatorRtv : public DX12DescriptorAllocator
	{
	public:
		DX12DescriptorAllocatorRtv(const uint32_t numDescriptors);

		uint32_t CreateRtv(const DX12Resource& buffer, const D3D12_RENDER_TARGET_VIEW_DESC* desc);

		D3D12_CPU_DESCRIPTOR_HANDLE CpuHandleFromRtvHandle(uint32_t handle) const;
	};

	class DX12DescriptorAllocatorDsv : public DX12DescriptorAllocator
	{
	public:
		DX12DescriptorAllocatorDsv(const uint32_t numDescriptors);

		uint32_t CreateDsv(const DX12Resource& buffer, const D3D12_DEPTH_STENCIL_VIEW_DESC* desc);

		D3D12_CPU_DESCRIPTOR_HANDLE CpuHandleFromDsvHandle(uint32_t handle) const;
	};

	class DX12DescriptorAllocatorSampler : public DX12DescriptorAllocator
	{
	public:
		DX12DescriptorAllocatorSampler(const uint32_t numDescriptors);

		uint32_t CreateSampler(const D3D12_SAMPLER_DESC* desc);
	};
}