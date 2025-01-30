#pragma once
#include "Renderer/D3D12/Common_DX12.hpp"

namespace Butterfly
{
	class DX12Resource;

	class BFView
	{
	protected:
		BFView(const DX12Resource& resource);
		const DX12Resource& m_resource;
	};

	class BFRenderTargetView : public BFView
	{
	public:
		BFRenderTargetView(const DX12Resource& resource, const D3D12_RENDER_TARGET_VIEW_DESC& desc);
		~BFRenderTargetView();

		uint32_t View() const { return m_viewIndex; }
		const D3D12_CPU_DESCRIPTOR_HANDLE& Handle() const { return m_handle; }

	private:
		D3D12_CPU_DESCRIPTOR_HANDLE m_handle;
		D3D12_RENDER_TARGET_VIEW_DESC m_desc;
		uint32_t m_viewIndex;
	};

	class BFDepthStencilView : public BFView
	{
	public:
		BFDepthStencilView(const DX12Resource& resource, const D3D12_DEPTH_STENCIL_VIEW_DESC& desc);
		~BFDepthStencilView();

		uint32_t View() const { return m_viewIndex; }
		const D3D12_CPU_DESCRIPTOR_HANDLE& Handle() const { return m_handle; }

	private:
		D3D12_CPU_DESCRIPTOR_HANDLE m_handle;
		D3D12_DEPTH_STENCIL_VIEW_DESC m_desc;
		uint32_t m_viewIndex;
	};

	class BFShaderResourceView : public BFView
	{
	public:
		BFShaderResourceView(const DX12Resource& resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc);
		~BFShaderResourceView();

		uint32_t View() const { return m_viewIndex; }

	private:
		D3D12_SHADER_RESOURCE_VIEW_DESC m_desc;
		uint32_t m_viewIndex;
	};

	class BFUniformBufferView : public BFView
	{
	public:
		BFUniformBufferView(const DX12Resource& resource, uint32_t sizeInBytes);
		~BFUniformBufferView();

		uint32_t View() const { return m_viewIndex; }

	private:
		D3D12_CPU_DESCRIPTOR_HANDLE m_handle;
		uint32_t m_viewIndex;
		uint32_t m_sizeInBytes;
	};
}