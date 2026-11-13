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
		uint32_t NumElements() const { return m_desc.NumElements; }

		D3D12Resource* Resource() const;
		ID3D12Resource2* DXResource() const;
		const BFShaderResourceView& SRV() const;

	private:
		D3D12Resource* m_resource;
		BFShaderResourceView* m_srv;
		BFStructuredBufferDesc m_desc;
	};

	struct BFTypedStructuredBufferDesc
	{
		uint32_t NumElements = 0;
		void* Data = nullptr;
		BFHeapType HeapType = BFHeapType::Default;
	};

	template <typename T>
	class BFTypedStructuredBuffer
	{
	public:
		BFTypedStructuredBuffer(const BFTypedStructuredBufferDesc& desc)
		{
			BFStructuredBufferDesc bufferDesc;
			bufferDesc.NumElements = desc.NumElements;
			bufferDesc.Stride = sizeof(T);
			bufferDesc.Data = desc.Data;
			bufferDesc.HeapType = desc.HeapType;
			bufferDesc.DebugName = "TypedStructuredBuffer::" + std::string(typeid(T).name());
			m_buffer = MakeRef<BFStructuredBuffer>(bufferDesc);
		}

		const BFShaderResourceView& SRV() const { return m_buffer->SRV(); }
		uint32_t NumElements() const { return m_buffer->NumElements(); }

		void Write()
		{
			m_buffer->Write(Data.data(), static_cast<uint32_t>(Data.size() * sizeof(T)), 0);
		}

		std::vector<T> Data;

	private:
		RefPtr<BFStructuredBuffer> m_buffer;
	};

	class BFConstantBuffer : public BFResource, private NonCopyable
	{
	public:
		BFConstantBuffer(uint32_t numBytes, const std::string& resourceTag);
		~BFConstantBuffer();

		const BFUniformBufferView& CBV() const;
		void Write(const void* src, uint32_t numBytes, uint32_t offset = 0);
		void Write(const std::vector<uint8_t>& data, uint32_t offset = 0);

	private:
		uint32_t m_numBytes;
		RefPtr<D3D12Resource> m_resource;
		uint8_t* m_mappedData;
		RefPtr<BFUniformBufferView> m_cbv;
	};

	template <typename T>
	class BFTypedConstantBuffer
	{
	public:
		BFTypedConstantBuffer()
			: m_buffer(sizeof(T), "TypedConstantBuffer::" + std::string(typeid(T).name()))
		{
		}

		const BFUniformBufferView& CBV() const { return m_buffer.CBV(); }

		void Write()
		{
			m_buffer.Write(&Data, sizeof(T));
		}

		T Data{};

	private:
		BFConstantBuffer m_buffer;
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
