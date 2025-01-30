#include "Renderer/D3D12/DescriptorAllocator_DX12.hpp"
#include "Renderer/D3D12/Resource_DX12.hpp"
#include "Renderer/D3D12/GraphicsAPI_DX12.hpp"

namespace Butterfly
{
	DX12DescriptorAllocator::DX12DescriptorAllocator(const D3D12_DESCRIPTOR_HEAP_DESC description, const std::wstring& resourceTag)
		: m_desc(description), m_incrementSize(DX12API()->Device()->GetDescriptorHandleIncrementSize(description.Type))
	{
		BF_PROFILE_EVENT();

		ThrowIfFailed(DX12API()->Device()->CreateDescriptorHeap(&m_desc, IID_PPV_ARGS(&m_heap)));
		m_heap->SetName(resourceTag.c_str());
		m_nextFreeIndex = 0;
		m_freedDiscriptors.reserve(64);
	}

	uint32_t DX12DescriptorAllocator::NextFreeHandle()
	{
		BF_PROFILE_EVENT();

		if (!m_freedDiscriptors.empty())
		{
			const uint32_t firstFreedValue = *m_freedDiscriptors.begin();
			m_freedDiscriptors.erase(m_freedDiscriptors.begin());
			return firstFreedValue;
		}

		return m_nextFreeIndex++;
	}

	void DX12DescriptorAllocator::FreeHandle(uint32_t handle)
	{
		BF_PROFILE_EVENT();

		CheckMsg(handle <= m_nextFreeIndex, "Discriptor handle index is higher than the existing index.");
		CheckMsg(std::find(m_freedDiscriptors.begin(), m_freedDiscriptors.end(), handle) == m_freedDiscriptors.end(), "Discriptor handle index is higher than the existing index.");

		m_freedDiscriptors.push_back(handle);
	}

	/////////////////////////
	// CbvSrvUav allocator.//
	/////////////////////////

	DX12DescriptorAllocatorCbvSrvUav::DX12DescriptorAllocatorCbvSrvUav(const uint32_t numDescriptors)
		: DX12DescriptorAllocator(
			D3D12_DESCRIPTOR_HEAP_DESC
			{
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
				numDescriptors,
				D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
				0 
			}, L"DescriptorAllocatorCbvSrvUav Heap")
	{
	}

	uint32_t DX12DescriptorAllocatorCbvSrvUav::CreateCbv(uint32_t numBytes, const DX12Resource& resource)
	{
		BF_PROFILE_EVENT();

		CheckMsg(m_nextFreeIndex <static_cast<uint32_t>(m_desc.NumDescriptors), "Discriptor heap has overflown.")
		CheckMsg(numBytes % 256 == 0, "CBV NumBytes must be 256 byte aligned.")
		CheckMsg(numBytes <= resource.BufferDescription.Width, 
			"CBV Virtual address overflow. Max NumBytes: %llu Requested NumBytes: %u",
			resource.BufferDescription.Width, numBytes)

		D3D12_CONSTANT_BUFFER_VIEW_DESC desc;
		desc.SizeInBytes = numBytes;
		desc.BufferLocation = resource.HwResource->GetGPUVirtualAddress();

		const uint32_t nextFreeValue = NextFreeHandle();

		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_heap->GetCPUDescriptorHandleForHeapStart();
		cpuHandle.ptr += m_incrementSize * nextFreeValue;
		DX12API()->Device()->CreateConstantBufferView(&desc, cpuHandle);
		return nextFreeValue;
	}

	uint32_t DX12DescriptorAllocatorCbvSrvUav::CreateSrv(const DX12Resource& resource, const D3D12_SHADER_RESOURCE_VIEW_DESC* description)
	{
		BF_PROFILE_EVENT();

		CheckMsg(m_nextFreeIndex < static_cast<uint32_t>(m_desc.NumDescriptors), "Discriptor heap has overflown.");

		const uint32_t nextFreeValue = NextFreeHandle();

		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_heap->GetCPUDescriptorHandleForHeapStart();
		cpuHandle.ptr += m_incrementSize * nextFreeValue;
		DX12API()->Device()->CreateShaderResourceView(resource.HwResource, description, cpuHandle);
		return nextFreeValue;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE DX12DescriptorAllocatorCbvSrvUav::GpuHandleFromSrvHandle(uint32_t handle) const
	{
		BF_PROFILE_EVENT();

		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_heap->GetGPUDescriptorHandleForHeapStart();
		gpuHandle.ptr += m_incrementSize * handle;
		return gpuHandle;
	}

	void DX12DescriptorAllocatorCbvSrvUav::AllocateDummy()
	{
		NextFreeHandle();
	}

	/////////////////////////
	// Rtv allocator.//
	/////////////////////////

	DX12DescriptorAllocatorRtv::DX12DescriptorAllocatorRtv(const uint32_t numDescriptors)
		: DX12DescriptorAllocator(
			D3D12_DESCRIPTOR_HEAP_DESC
			{
				D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
				numDescriptors,
				D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
				0
			}, L"DescriptorAllocatorRtv Heap")
	{
	}

	uint32_t DX12DescriptorAllocatorRtv::CreateRtv(const DX12Resource& resource, const D3D12_RENDER_TARGET_VIEW_DESC* desc)
	{
		BF_PROFILE_EVENT();

		CheckMsg(m_nextFreeIndex < static_cast<uint32_t>(m_desc.NumDescriptors), "Discriptor heap buffer has overflown.");

		const uint32_t nextFreeValue = NextFreeHandle();

		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_heap->GetCPUDescriptorHandleForHeapStart();
		cpuHandle.ptr += m_incrementSize * nextFreeValue;
		DX12API()->Device()->CreateRenderTargetView(resource.HwResource, desc, cpuHandle);
		return nextFreeValue;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DX12DescriptorAllocatorRtv::CpuHandleFromRtvHandle(uint32_t handle) const
	{
		BF_PROFILE_EVENT();

		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_heap->GetCPUDescriptorHandleForHeapStart();
		cpuHandle.ptr += m_incrementSize * handle;
		return cpuHandle;
	}

	/////////////////////////
	// Dsv allocator.//
	/////////////////////////

	DX12DescriptorAllocatorDsv::DX12DescriptorAllocatorDsv(const uint32_t numDescriptors)
		: DX12DescriptorAllocator(
			D3D12_DESCRIPTOR_HEAP_DESC
			{
				D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
				numDescriptors,
				D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
				0
			}, L"DescriptorAllocatorDsv Heap")
	{
	}

	uint32_t DX12DescriptorAllocatorDsv::CreateDsv(const DX12Resource& resource, const D3D12_DEPTH_STENCIL_VIEW_DESC* desc)
	{
		BF_PROFILE_EVENT();

		CheckMsg(m_nextFreeIndex < static_cast<uint32_t>(m_desc.NumDescriptors), "Discriptor heap buffer has overflown.");

		const uint32_t nextFreeValue = NextFreeHandle();

		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_heap->GetCPUDescriptorHandleForHeapStart();
		cpuHandle.ptr += m_incrementSize * nextFreeValue;
		DX12API()->Device()->CreateDepthStencilView(resource.HwResource, desc, cpuHandle);
		return nextFreeValue;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DX12DescriptorAllocatorDsv::CpuHandleFromDsvHandle(uint32_t handle) const
	{
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_heap->GetCPUDescriptorHandleForHeapStart();
		cpuHandle.ptr += m_incrementSize * handle;
		return cpuHandle;
	}

	/////////////////////////
	// Sampler allocator.//
	/////////////////////////

	DX12DescriptorAllocatorSampler::DX12DescriptorAllocatorSampler(const uint32_t numDescriptors)
		: DX12DescriptorAllocator(
			D3D12_DESCRIPTOR_HEAP_DESC
			{
				D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
				numDescriptors,
				D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
				0
			}, L"DescriptorAllocatorSampler Heap")
	{
	}

	uint32_t DX12DescriptorAllocatorSampler::CreateSampler(const D3D12_SAMPLER_DESC* desc)
	{
		BF_PROFILE_EVENT();

		CheckMsg(m_nextFreeIndex < static_cast<uint32_t>(m_desc.NumDescriptors), "Discriptor heap buffer has overflown.");

		const uint32_t nextFreeValue = NextFreeHandle();

		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_heap->GetCPUDescriptorHandleForHeapStart();
		cpuHandle.ptr += m_incrementSize * nextFreeValue;
		DX12API()->Device()->CreateSampler(desc, cpuHandle);
		return nextFreeValue;
	}
}