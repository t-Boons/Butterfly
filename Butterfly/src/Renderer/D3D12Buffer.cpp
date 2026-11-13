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
            view.BufferLocation = buffer.DXResource()->GetGPUVirtualAddress();
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
            m_resource->Transition(transitionList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

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

    D3D12Resource* BFStructuredBuffer::Resource() const
    {
        return m_resource;
    }

    ID3D12Resource2* BFStructuredBuffer::DXResource() const
    {
        return m_resource->HwResource;
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


    // BFConstantBuffer

    BFConstantBuffer::BFConstantBuffer(uint32_t numBytes, const std::string& resourceTag)
        : m_numBytes(0), m_mappedData(nullptr), m_cbv(nullptr)
    {
        BF_PROFILE_EVENT();

        const uint32_t alignedSize = Align256(numBytes);
		m_numBytes = alignedSize;
        D3D12Resource* resource = DX12ResourceBuilder()
            .HeapType(D3D12_HEAP_TYPE_UPLOAD)
            .Buffer(alignedSize)
            .SetName(resourceTag)
            .Create();

		m_resource = RefPtr<D3D12Resource>(resource);


        m_mappedData = static_cast<uint8_t*>(m_resource->Map());

        m_cbv = MakeRef<BFUniformBufferView>(*m_resource, alignedSize, 0);
    }

    BFConstantBuffer::~BFConstantBuffer()
    {
        BF_PROFILE_EVENT();

        m_cbv.reset();
        m_resource.reset();
    }

    const BFUniformBufferView& BFConstantBuffer::CBV() const
    {
        return *m_cbv;
    }

    void BFConstantBuffer::Write(const void* src, uint32_t numBytes, uint32_t offset)
    {
        BF_CORE_ASSERT(offset + numBytes <= m_numBytes, "Not enough space in uniform buffer to write data.");
        memcpy(static_cast<void*>(m_mappedData + offset), src, numBytes);
    }

    void BFConstantBuffer::Write(const std::vector<uint8_t>& data, uint32_t offset)
    {
        Write(data.data(), static_cast<uint32_t>(data.size()), offset);
    }
}