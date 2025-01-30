#pragma once
#include "D3D12/Common_DX12.hpp"
#include "Resource.hpp"

namespace Butterfly
{
	class DX12Resource;

	class BFShaderResourceView;
	class BFUniformBufferView;

	class BFStructuredBuffer : public BFResource, private NonCopyable
	{
	public:
		BFStructuredBuffer(const void* src, uint32_t numBytes, D3D12_SHADER_RESOURCE_VIEW_DESC* srv, const std::string& resourceTag);
		~BFStructuredBuffer();

		uint32_t NumBytes() const { return m_numBytes; }
		void Write(const void* src, uint32_t numBytes);

		ID3D12Resource2* Resource() const;
		DX12Resource& DXResource() const;
		const BFShaderResourceView& SRV() const;

	private:
		DX12Resource* m_resource;
		uint32_t m_numBytes;
		BFShaderResourceView* m_srv;
	};

	class BFUniformBuffer : public BFResource, private NonCopyable
	{
	public:
		BFUniformBuffer(uint32_t numBytes, const std::string& resourceTag);
		~BFUniformBuffer();

		const BFUniformBufferView& CBV() const;
		void Write(const void* src, uint32_t numBytes);

		uint32_t m_numBytes;
		DX12Resource* m_resource;
		BFUniformBufferView* m_cbv;
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
		DX12Resource* m_resource;
		D3D12_INDEX_BUFFER_VIEW m_view;
	};
}