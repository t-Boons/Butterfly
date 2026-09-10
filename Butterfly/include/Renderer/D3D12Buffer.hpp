#pragma once
#include "D3D12/D3D12Common.hpp"
#include "Resource.hpp"

namespace Butterfly
{
	class D3D12Resource;

	class BFShaderResourceView;
	class BFUniformBufferView;

	enum class BFHeapType
	{
		Default, Upload
	};

	struct BFStructuredBufferDesc
	{
		uint32_t NumElements = 0;
		uint32_t Stride = 0;
		void* Data = nullptr;
		BFHeapType HeapType = BFHeapType::Default;
		std::string DebugName = "StructuredBuffer";
	};

	class BFStructuredBuffer : public BFResource, private NonCopyable
	{
	public:
		BFStructuredBuffer(const BFStructuredBufferDesc& bufferDesc);
		~BFStructuredBuffer();

		uint32_t NumBytes() const { return m_desc.NumElements * m_desc.Stride; }
		void Write(const void* src, uint32_t numBytes, uint32_t offset = 0);

		ID3D12Resource2* Resource() const;
		D3D12Resource& DXResource() const;
		const BFShaderResourceView& SRV() const;

	private:
		D3D12Resource* m_resource;
		BFShaderResourceView* m_srv;
		BFStructuredBufferDesc m_desc;
	};

	class BFUniformBuffer : public BFResource, private NonCopyable
	{
	public:
		BFUniformBuffer(uint32_t numBytes, const std::string& resourceTag);
		~BFUniformBuffer();

		uint32_t AllocView(uint32_t sizeInBytes);

		const RefPtr<BFUniformBufferView> GetView(uint32_t viewIndex) const;
		uint32_t GetViewOffset(uint32_t viewIndex) const;
		void Write(const void* src, uint32_t numBytes, uint32_t viewIndex);

	private:
		uint32_t m_numBytes;
		uint32_t m_bytesAllocated;
		RefPtr<D3D12Resource> m_resource;
		std::vector<RefPtr<BFUniformBufferView>> m_cbvs;
		void* m_mappedData;
	};

	class BFIndexBuffer : public BFResource, private NonCopyable
	{
	public:
		BFIndexBuffer(const void* src, uint32_t numElements, DXGI_FORMAT format, const std::string& resourceTag);
		~BFIndexBuffer();

		const uint32_t NumElements() const { return m_numElements; }
		const D3D12_INDEX_BUFFER_VIEW& IBV() const { return m_view; }

	private:
		uint32_t m_numElements;
		D3D12Resource* m_resource;
		D3D12_INDEX_BUFFER_VIEW m_view;
	};
}
