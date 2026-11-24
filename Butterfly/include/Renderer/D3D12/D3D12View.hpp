#pragma once
#include "Renderer/D3D12/D3D12Common.hpp"
#include "Renderer/D3D12/D3D12Resource.hpp"

namespace Butterfly
{
	class BFView : public NonCopyable
	{
	public:
		D3D12_RESOURCE_STATES RequiredState() const { return m_requiredState; }
		bool RequiresTransition() const;
		D3D12SubresourceRange SubresourceRange() const { return m_subresourceRange; }
		D3D12Resource& Resource() const { return m_resource; }
		uint32_t View() const { return m_viewIndex; }

	protected:
		BFView(D3D12Resource& resource);
		
		D3D12Resource& m_resource;
		D3D12_RESOURCE_STATES m_requiredState = D3D12_RESOURCE_STATE_COMMON;
		D3D12SubresourceRange m_subresourceRange;
		uint32_t m_viewIndex = INT_MAX;
	};

	class BFRenderTargetView : public BFView
	{
	public:
		BFRenderTargetView(D3D12Resource& resource, const D3D12_RENDER_TARGET_VIEW_DESC& desc);
		~BFRenderTargetView();

		const D3D12_CPU_DESCRIPTOR_HANDLE& Handle() const { return m_handle; }

	private:
		D3D12_CPU_DESCRIPTOR_HANDLE m_handle;
		D3D12_RENDER_TARGET_VIEW_DESC m_desc;
	};

	class BFDepthStencilView : public BFView
	{
	public:
		BFDepthStencilView(D3D12Resource& resource, const D3D12_DEPTH_STENCIL_VIEW_DESC& desc);
		~BFDepthStencilView();

		const D3D12_CPU_DESCRIPTOR_HANDLE& Handle() const { return m_handle; }

	private:
		D3D12_CPU_DESCRIPTOR_HANDLE m_handle;
		D3D12_DEPTH_STENCIL_VIEW_DESC m_desc;
	};

	class BFShaderResourceView : public BFView
	{
	public:
		BFShaderResourceView(D3D12Resource& resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc);
		~BFShaderResourceView();

	private:
		D3D12_SHADER_RESOURCE_VIEW_DESC m_desc;
	};

	class BFUniformBufferView : public BFView
	{
	public:
		BFUniformBufferView(D3D12Resource& resource, uint32_t sizeInBytes, uint32_t offset);
		~BFUniformBufferView();

		uint32_t Offset() const { return m_offset; }
		uint32_t NumBytes() const { return m_sizeInBytes; }

	private:
		D3D12_CPU_DESCRIPTOR_HANDLE m_handle;
		uint32_t m_sizeInBytes;
		uint32_t m_offset;
	};


	class BFUnorderedAccessView : public BFView
	{
	public:
		BFUnorderedAccessView(D3D12Resource& resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc);
		~BFUnorderedAccessView();

	private:
		D3D12_CPU_DESCRIPTOR_HANDLE m_handle;
		D3D12_UNORDERED_ACCESS_VIEW_DESC m_desc;
	};
}