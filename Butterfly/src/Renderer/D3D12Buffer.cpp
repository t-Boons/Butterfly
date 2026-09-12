#include "Renderer/D3D12Buffer.hpp"
#include "Renderer/D3D12/D3D12GraphicsAPI.hpp"
#include "Renderer/D3D12/D3D12View.hpp"
#include "Renderer/D3D12/D3D12CommandQueue.hpp"
#include "Renderer/D3D12/D3D12CommandList.hpp"
#include "Renderer/D3D12/D3D12Resource.hpp"

namespace Butterfly
{
    namespace Utils
    {
        inline const D3D12_INDEX_BUFFER_VIEW IndexBufferViewFromBuffer(const BFStructuredBuffer& buffer)
        {
            BF_PROFILE_EVENT();

            D3D12_INDEX_BUFFER_VIEW view;
            view.BufferLocation = buffer.Resource()->GetGPUVirtualAddress();
            view.SizeInBytes = buffer.NumBytes();
            view.Format = DXGI_FORMAT_R32_UINT;
            return view;
        }
    }

    BFStructuredBuffer::BFStructuredBuffer(const BFStructuredBufferDesc& bufferDesc)
        : m_srv(nullptr), m_desc(bufferDesc)
    {
        BF_PROFILE_EVENT();

        BF_CORE_ASSERT(!(m_desc.HeapType == BFHeapType::Default && !m_desc.Data), "BFStructuredBuffer::BFStructuredBuffer Structured buffer with HeapType::Default, cannot have its Data be null");

        D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;

        if (bufferDesc.HeapType == BFHeapType::Upload) heapType = D3D12_HEAP_TYPE_UPLOAD;
        if (bufferDesc.HeapType == BFHeapType::Default) heapType = D3D12_HEAP_TYPE_DEFAULT;

        const uint32_t numBytes = bufferDesc.NumElements * bufferDesc.Stride;

        m_resource = DX12ResourceBuilder()
            .HeapType(heapType)
            .InitialState(D3D12_RESOURCE_STATE_COMMON)
            .Buffer(numBytes)
            .SetName(bufferDesc.DebugName)
            .Create();

        if (bufferDesc.Data)
        {
            D3D12Resource* uploadResource = DX12ResourceBuilder()
                .HeapType(D3D12_HEAP_TYPE_UPLOAD)
                .InitialState(D3D12_RESOURCE_STATE_COPY_SOURCE)
                .Buffer(numBytes)
                .SetName(bufferDesc.DebugName + " intermediate upload resource.")
                .Create()
                ->Write(bufferDesc.Data, numBytes);

            D3D12CommandList copyList(D3D12_COMMAND_LIST_TYPE_COPY);

            m_resource->Transition(copyList, D3D12_RESOURCE_STATE_COPY_DEST);
            copyList.List()->CopyResource(m_resource->HwResource, uploadResource->HwResource);
            copyList.Close();
            D3D12API()->Queue(QueueType::Copy)->Execute(copyList);
            D3D12API()->Queue(QueueType::Copy)->WaitForFence();

            D3D12CommandList transitionList;
            m_resource->Transition(transitionList, D3D12_RESOURCE_STATE_GENERIC_READ);

            transitionList.Close();
            D3D12API()->Queue(QueueType::Direct)->Execute(transitionList);
            D3D12API()->Queue(QueueType::Direct)->WaitForFence();

            FREE(uploadResource);
        }

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
        srvDesc.Buffer.StructureByteStride = bufferDesc.Stride;
        srvDesc.Buffer.NumElements = bufferDesc.NumElements;

        m_srv = new BFShaderResourceView(*m_resource, srvDesc);
    }

    BFStructuredBuffer::~BFStructuredBuffer()
    {
        BF_PROFILE_EVENT()

        FREE(m_srv);
        FREE(m_resource);
    }

    void BFStructuredBuffer::Write(const void* src, uint32_t numBytes, uint32_t offset)
    {
        m_resource->Write(src, numBytes, offset);
    }

    ID3D12Resource2* BFStructuredBuffer::Resource() const
    {
        return m_resource->HwResource;
    }

    D3D12Resource& BFStructuredBuffer::DXResource() const
    {
        return *m_resource;
    }

    const BFShaderResourceView& BFStructuredBuffer::SRV() const
    {
        BF_CORE_ASSERT(m_srv != nullptr, "%s", "Buffer does not have an SRV");
        return *m_srv;
    }


    // BFIndexBuffer

    BFIndexBuffer::BFIndexBuffer(const void* src, uint32_t numElements, DXGI_FORMAT format, const std::string& resourceTag)
        : m_numElements(numElements)
    {
        BF_PROFILE_EVENT();

        uint32_t bytesPerElement = 0;
        switch (format)
        {
        case DXGI_FORMAT_R32_UINT: bytesPerElement = 4;                 break;
        case DXGI_FORMAT_R16_UINT: bytesPerElement = 2;                 break;
        default: BF_CORE_LOG_CRITICAL("%s", "Invalid IndexBuffer format.");    break;
        }

        const uint32_t numBytes = bytesPerElement * numElements;

        D3D12Resource* uploadResource = DX12ResourceBuilder()
            .HeapType(D3D12_HEAP_TYPE_UPLOAD)
            .InitialState(D3D12_RESOURCE_STATE_COPY_SOURCE)
            .IndexBuffer(numBytes)
            .SetName(resourceTag + " intermediate upload resource.")
            .Create()
            ->Write(src, numBytes);


        m_resource = DX12ResourceBuilder()
            .HeapType(D3D12_HEAP_TYPE_DEFAULT)
            .IndexBuffer(numBytes)
            .SetName(resourceTag)
            .InitialState(D3D12_RESOURCE_STATE_COMMON)
            .Create();

        D3D12CommandList copyList(D3D12_COMMAND_LIST_TYPE_COPY);

        m_resource->Transition(copyList, D3D12_RESOURCE_STATE_COPY_DEST);
        copyList.List()->CopyResource(m_resource->HwResource, uploadResource->HwResource);
        copyList.Close();
        D3D12API()->Queue(QueueType::Copy)->Execute(copyList);
        D3D12API()->Queue(QueueType::Copy)->WaitForFence();

        D3D12CommandList transitionList;
        m_resource->Transition(transitionList, D3D12_RESOURCE_STATE_INDEX_BUFFER);

        transitionList.Close();
        D3D12API()->Queue(QueueType::Direct)->Execute(transitionList);
        D3D12API()->Queue(QueueType::Direct)->WaitForFence();

        m_view.BufferLocation = m_resource->HwResource->GetGPUVirtualAddress();
        m_view.SizeInBytes = numBytes;
        m_view.Format = format;

        FREE(uploadResource);
    }

    BFIndexBuffer::~BFIndexBuffer()
    {
        BF_PROFILE_EVENT()

            FREE(m_resource);
    }


    // BFUniformBuffer

    BFUniformBuffer::BFUniformBuffer(uint32_t numBytes, const std::string& resourceTag)
        : m_numBytes(numBytes), m_bytesAllocated(0)
    {
        BF_PROFILE_EVENT();

        D3D12Resource* resource = DX12ResourceBuilder()
            .HeapType(D3D12_HEAP_TYPE_UPLOAD)
            .Buffer(numBytes)
            .SetName(resourceTag)
            .Create();

		m_resource = RefPtr<D3D12Resource>(resource);


        m_mappedData = m_resource->Map();
    }

    BFUniformBuffer::~BFUniformBuffer()
    {
        BF_PROFILE_EVENT()

        m_cbvs.clear();
        m_resource.reset();
    }

    uint32_t BFUniformBuffer::GetOrCreateView(uint32_t sizeInBytes, uint32_t hashedName)
    {
        BF_PROFILE_EVENT();

		if (m_cbvs.find(hashedName) != m_cbvs.end())
		{
			return hashedName;
		}

		const uint32_t alignedSize = Align256(sizeInBytes);
        BF_CORE_ASSERT(m_bytesAllocated + alignedSize <= m_numBytes, "Not enough space in uniform buffer to allocate view.");

        uint32_t offset = m_bytesAllocated;
        m_bytesAllocated += alignedSize;
        RefPtr<BFUniformBufferView> view = MakeRef<BFUniformBufferView>(*m_resource, alignedSize, offset);
        m_cbvs[hashedName] = view;
        return hashedName;
    }

    const RefPtr<BFUniformBufferView>& BFUniformBuffer::GetView(uint32_t viewName) const
    {
        BF_CORE_ASSERT(m_cbvs.find(viewName) != m_cbvs.end(), "Invalid view name");
        return m_cbvs.at(viewName);
    }

    uint32_t BFUniformBuffer::GetViewOffset(uint32_t viewName) const
    {
        return GetView(viewName)->Offset();
    }

    void BFUniformBuffer::Write(const void* src, uint32_t numBytes, uint32_t viewName)
    {
        BF_CORE_ASSERT(numBytes <= GetView(viewName)->NumBytes(), "Not enough space in uniform buffer view to write data.");

        memcpy(m_mappedData, src, numBytes);
    }
}