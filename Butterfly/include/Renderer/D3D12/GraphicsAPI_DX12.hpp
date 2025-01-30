#pragma once
#include "Common_DX12.hpp"
#include "Renderer/GraphicsAPI.hpp"
#include "Types_DX12.hpp"

namespace Butterfly
{
	class DX12GraphicsAPI : public GraphicsAPI
	{
	public:
		DX12GraphicsAPI() = default;
		~DX12GraphicsAPI();

		ID3D12Device4* Device() const { return m_device; }
		IDXGIFactory7* Factory() const { return m_factory; }

		class DX12CommandQueue* Queue(QueueType type) const { return m_queues[static_cast<uint32_t>(type)]; }
		class DX12DescriptorAllocatorCbvSrvUav* DescriptorAllocatorSrvCbvUav() const { return m_descriptorAllocatorSrvCbvUav; }
		class DX12DescriptorAllocatorSampler* DescriptorAllocatorSampler() const { return m_descriptorAllocatorsSampler; }
		class DX12DescriptorAllocatorRtv* DescriptorAllocatorRtv() const { return m_descriptorAllocatorsRtv; }
		class DX12DescriptorAllocatorDsv* DescriptorAllocatorDsv() const { return m_descriptorAllocatorsDsv; }

		ID3D12RootSignature* BindlessRootSignature() const { return m_bindlessRootSignature; }
	
		IDxcCompiler3* ShaderCompiler() const { return m_shaderCompiler; }
		IDxcUtils* ShaderUtils() const { return m_utils; }

	public:
		virtual RefPtr<CommandList> CreateCommandList(QueueType type) const override;




	protected:
		virtual void _Init(bool enableDebug) override;

	public:
		ID3D12Device4* m_device;
		IDXGIFactory7* m_factory;

		class DX12CommandQueue* m_queues[static_cast<uint32_t>(QueueType::Num)];
		class DX12DescriptorAllocatorCbvSrvUav*	m_descriptorAllocatorSrvCbvUav;
		class DX12DescriptorAllocatorRtv*			m_descriptorAllocatorsRtv;
		class DX12DescriptorAllocatorDsv*			m_descriptorAllocatorsDsv;
		class DX12DescriptorAllocatorSampler*		m_descriptorAllocatorsSampler;

		ID3D12RootSignature* m_bindlessRootSignature;

		IDxcCompiler3* m_shaderCompiler;
		IDxcUtils* m_utils;
	};

	inline DX12GraphicsAPI* DX12API()
	{
		return reinterpret_cast<DX12GraphicsAPI*>(Graphics);
	}
}