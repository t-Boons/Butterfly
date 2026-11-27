#pragma once
#include "D3D12/D3D12Common.hpp"
#include "Resource.hpp"

namespace Butterfly
{
	class D3D12Resource;

	class ShaderResourceView;
	class UniformBufferView;

	enum class HeapType
	{
		Default, Upload
	};

	struct StructuredBufferDesc
	{
		uint32_t NumElements = 0;
		uint32_t Stride = 0;
		void* Data = nullptr;
		HeapType HeapType = HeapType::Default;
		std::string DebugName = "StructuredBuffer";

		bool operator ==(const StructuredBufferDesc& other) const = default;
	};

	class StructuredBuffer : public Resource, private NonCopyable
	{
	public:
		StructuredBuffer(const StructuredBufferDesc& bufferDesc);
		~StructuredBuffer();

		uint32_t NumBytes() const { return m_desc.NumElements * m_desc.Stride; }
		void Write(const void* src, uint32_t numBytes, uint32_t offset = 0);
		uint32_t NumElements() const { return m_desc.NumElements; }

		D3D12Resource* Resource() const;
		ID3D12Resource2* HWResource() const;
		const ShaderResourceView& SRV() const;

	private:
		D3D12Resource* m_resource;
		ShaderResourceView* m_srv;
		StructuredBufferDesc m_desc;
	};

	struct TypedStructuredBufferDesc
	{
		uint32_t NumElements = 0;
		void* Data = nullptr;
		HeapType HeapType = HeapType::Default;
	};

	template <typename T>
	class TypedStructuredBuffer
	{
	public:
		TypedStructuredBuffer(const TypedStructuredBufferDesc& desc)
		{
			StructuredBufferDesc bufferDesc;
			bufferDesc.NumElements = desc.NumElements;
			bufferDesc.Stride = sizeof(T);
			bufferDesc.Data = desc.Data;
			bufferDesc.HeapType = desc.HeapType;
			bufferDesc.DebugName = "TypedStructuredBuffer::" + std::string(typeid(T).name());
			m_buffer = MakeRef<StructuredBuffer>(bufferDesc);
		}

		const ShaderResourceView& SRV() const { return m_buffer->SRV(); }
		uint32_t NumElements() const { return m_buffer->NumElements(); }

		void Write()
		{
			m_buffer->Write(Data.data(), static_cast<uint32_t>(Data.size() * sizeof(T)), 0);
		}

		std::vector<T> Data;

	private:
		RefPtr<StructuredBuffer> m_buffer;
	};

	class ConstantBuffer : public Resource, private NonCopyable
	{
	public:
		ConstantBuffer(uint32_t numBytes, const std::string& resourceTag);
		~ConstantBuffer();

		const UniformBufferView& CBV() const;
		void Write(const void* src, uint32_t numBytes, uint32_t offset = 0);
		void Write(const std::vector<uint8_t>& data, uint32_t offset = 0);

	private:
		uint32_t m_numBytes;
		RefPtr<D3D12Resource> m_resource;
		uint8_t* m_mappedData;
		RefPtr<UniformBufferView> m_cbv;
	};

	template <typename T>
	class TypedConstantBuffer
	{
	public:
		TypedConstantBuffer()
			: m_buffer(sizeof(T), "TypedConstantBuffer::" + std::string(typeid(T).name()))
		{
		}

		const UniformBufferView& CBV() const { return m_buffer.CBV(); }

		void Write()
		{
			m_buffer.Write(&Data, sizeof(T));
		}

		T Data{};

	private:
		ConstantBuffer m_buffer;
	};

	class IndexBuffer : public Resource, private NonCopyable
	{
	public:
		IndexBuffer(const void* src, uint32_t numElements, DXGI_FORMAT format, const std::string& resourceTag);
		~IndexBuffer();

		const uint32_t NumElements() const { return m_numElements; }
		const D3D12_INDEX_BUFFER_VIEW& IBV() const { return m_view; }

	private:
		uint32_t m_numElements;
		D3D12Resource* m_resource;
		D3D12_INDEX_BUFFER_VIEW m_view;
	};
}
