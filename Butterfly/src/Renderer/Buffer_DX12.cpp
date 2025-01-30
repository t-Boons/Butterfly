#include "Renderer/Buffer_DX12.hpp"
#include "Renderer/D3D12/GraphicsAPI_DX12.hpp"
#include "Renderer/D3D12/View_DX12.hpp"
#include "Renderer/D3D12/CommandQueue_DX12.hpp"
#include "Renderer/D3D12/CommandList_DX12.hpp"
#include "Renderer/D3D12/Resource_DX12.hpp"

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

    BFStructuredBuffer::BFStructuredBuffer(const void* src, uint32_t numBytes, D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc, const std::string& resourceTag)
        : m_srv(nullptr)
    {
        BF_PROFILE_EVENT();

        CheckMsg(src, "Structured buffer pointer is nullptr.");

        m_numBytes = numBytes;

        DX12Resource* uploadResource = DX12ResourceBuilder()
            .HeapType(D3D12_HEAP_TYPE_UPLOAD)
            .InitialState(D3D12_RESOURCE_STATE_COPY_SOURCE)
            .Buffer(numBytes)
            .SetName(resourceTag + " intermediate upload resource.")
            .Create()
            ->Write(src, numBytes);


        m_resource = DX12ResourceBuilder()
            .HeapType(D3D12_HEAP_TYPE_DEFAULT)
            .InitialState(D3D12_RESOURCE_STATE_COMMON)
            .Buffer(numBytes)
            .SetName(resourceTag)   
            .Create();

        DX12CommandList copyList(D3D12_COMMAND_LIST_TYPE_COPY);

        m_resource->Transition(copyList, D3D12_RESOURCE_STATE_COPY_DEST);
        copyList.List()->CopyResource(m_resource->HwResource, uploadResource->HwResource);
        copyList.Close();
        DX12API()->Queue(QueueType::Copy)->Execute(copyList);
        DX12API()->Queue(QueueType::Copy)->WaitForFence();

        DX12CommandList transitionList;
        m_resource->Transition(transitionList, D3D12_RESOURCE_STATE_GENERIC_READ);

        transitionList.Close();
        DX12API()->Queue(QueueType::Direct)->Execute(transitionList);
        DX12API()->Queue(QueueType::Direct)->WaitForFence();

        if (srvDesc) m_srv = new BFShaderResourceView(*m_resource, *srvDesc);


        FREE(uploadResource);
    }

    BFStructuredBuffer::~BFStructuredBuffer()
    {
        BF_PROFILE_EVENT()

        FREE(m_srv);
        FREE(m_resource);
    }

    void BFStructuredBuffer::Write(const void* src, uint32_t numBytes)
    {
        m_resource->Write(src, numBytes);
    }

    ID3D12Resource2* BFStructuredBuffer::Resource() const
    { 
        return m_resource->HwResource; 
    }

    DX12Resource& BFStructuredBuffer::DXResource() const 
    { 
        return *m_resource; 
    }

    const BFShaderResourceView& BFStructuredBuffer::SRV() const
    {
        CheckMsg(m_srv != nullptr, "%s", "Buffer does not have an SRV");
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
        default: Log::Critical("%s", "Invalid IndexBuffer format.");    break;
        }

        const uint32_t numBytes = bytesPerElement * numElements;

        DX12Resource* uploadResource = DX12ResourceBuilder()
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

        DX12CommandList copyList(D3D12_COMMAND_LIST_TYPE_COPY);

        m_resource->Transition(copyList, D3D12_RESOURCE_STATE_COPY_DEST);
        copyList.List()->CopyResource(m_resource->HwResource, uploadResource->HwResource);
        copyList.Close();
        DX12API()->Queue(QueueType::Copy)->Execute(copyList);
        DX12API()->Queue(QueueType::Copy)->WaitForFence();

        DX12CommandList transitionList;
        m_resource->Transition(transitionList, D3D12_RESOURCE_STATE_INDEX_BUFFER);

        transitionList.Close();
        DX12API()->Queue(QueueType::Direct)->Execute(transitionList);
        DX12API()->Queue(QueueType::Direct)->WaitForFence();

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
        : m_numBytes(numBytes), m_cbv(nullptr)
    {
        BF_PROFILE_EVENT();

        m_resource = DX12ResourceBuilder()
            .HeapType(D3D12_HEAP_TYPE_UPLOAD)
            .Buffer(numBytes)
            .SetName(resourceTag)
            .Create();

        m_cbv = new BFUniformBufferView(*m_resource, numBytes);
    }

    BFUniformBuffer::~BFUniformBuffer()
    {
        BF_PROFILE_EVENT()

        FREE(m_cbv);
        FREE(m_resource);
    }

    const BFUniformBufferView& BFUniformBuffer::CBV() const
    {
        return *m_cbv;
    }

    void BFUniformBuffer::Write(const void* src, uint32_t numBytes)
	{
        BF_PROFILE_EVENT();

		CheckMsg(m_numBytes >= numBytes, "Byte overflow in UniformBuffer.");
		m_resource->Write(src, numBytes);
	}
}