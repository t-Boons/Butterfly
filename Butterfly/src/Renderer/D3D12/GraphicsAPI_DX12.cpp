#include "Renderer/D3D12/GraphicsAPI_DX12.hpp"
#include "Renderer/D3D12/CommandQueue_DX12.hpp"
#include "Renderer/D3D12/DescriptorAllocator_DX12.hpp"
#include "Renderer/D3D12/CommandList_DX12.hpp"

namespace Butterfly
{
	inline static D3D12_MESSAGE_ID SUPRESSED_WARNINGS[] =
	{
		D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE
	};

	inline static D3D12_MESSAGE_SEVERITY ALLOWED_MESSAGES[] =
	{
		D3D12_MESSAGE_SEVERITY_CORRUPTION,
		D3D12_MESSAGE_SEVERITY_ERROR,
		D3D12_MESSAGE_SEVERITY_WARNING
	};

	static void DebugCallback(D3D12_MESSAGE_CATEGORY, D3D12_MESSAGE_SEVERITY severity,
		D3D12_MESSAGE_ID, const char* desc, void*)
	{
		switch (severity)
		{
		case D3D12_MESSAGE_SEVERITY_INFO:
			Log::Info("D3D12 -> %s", desc);
			break;
		case D3D12_MESSAGE_SEVERITY_WARNING:
			Log::Warn("D3D12 -> %s", desc);
			break;
		case D3D12_MESSAGE_SEVERITY_ERROR:
			Log::Critical("D3D12 -> %s", desc);
			break;
		default:
			Log::Critical("D3D12 -> %s", desc);
		}
	}

	RefPtr<CommandList> DX12GraphicsAPI::CreateCommandList(QueueType type) const
	{
		RefPtr<DX12CommandList> list = MakeRef<DX12CommandList>(DXQueueTypeFromQueueType(type));
		return std::dynamic_pointer_cast<CommandList>(list);
	}

	void DX12GraphicsAPI::_Init(bool enableDebug)
	{
		BF_PROFILE_EVENT();

		API = GraphicsAPIType::D3D12;

		// Enable or disable the debug layer.
		ComPtr<ID3D12Debug6> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			if (enableDebug)
			{
				debugController->EnableDebugLayer();
				debugController->SetEnableGPUBasedValidation(TRUE);
			}
			else
			{
				debugController->DisableDebugLayer();
				debugController->SetEnableGPUBasedValidation(FALSE);
			}
		}

		ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&m_factory)));

		// Create the device.
		ComPtr<IDXGIAdapter1> adapter;
		for (UINT adapterIndex = 0; m_factory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND; ++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);
			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				continue;
			}

			ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device)));

			Log::Info("Initialized DirectX 12");
			Log::Info("	GPU:  %ls", desc.Description);
			Log::Info("	VRAM: %i MB", static_cast<int>(desc.DedicatedVideoMemory / (1024 * 1024)));
			if (enableDebug) Log::Warn("	Debug layer enabled.");

			if (m_device) break;
		}

		// Push debug filters.
		D3D12_INFO_QUEUE_FILTER filter = {};
		filter.DenyList.NumIDs = _countof(SUPRESSED_WARNINGS);
		filter.DenyList.pIDList = SUPRESSED_WARNINGS;
		filter.AllowList.NumSeverities = _countof(ALLOWED_MESSAGES);
		filter.AllowList.pSeverityList = ALLOWED_MESSAGES;

		ComPtr<ID3D12InfoQueue1> infoQueue;
		if (SUCCEEDED(m_device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {

			infoQueue->PushStorageFilter(&filter);

			auto flag = D3D12_MESSAGE_CALLBACK_FLAG_NONE;
			DWORD cookie = 0;
			void* ctx = nullptr;
			infoQueue->RegisterMessageCallback(&DebugCallback, flag, ctx, &cookie);
		}

		// Allocate descriptor allocators.
		m_descriptorAllocatorSrvCbvUav = new DX12DescriptorAllocatorCbvSrvUav(4096);
		m_descriptorAllocatorsRtv = new DX12DescriptorAllocatorRtv(64);
		m_descriptorAllocatorsDsv = new DX12DescriptorAllocatorDsv(64);
		m_descriptorAllocatorsSampler = new DX12DescriptorAllocatorSampler(2048);

		// Allocate queues.
		m_queues[static_cast<uint32_t>(QueueType::Direct)] = new DX12CommandQueue(DXQueueTypeFromQueueType(QueueType::Direct), "DX12CommandQueue::DirectQueue");
		m_queues[static_cast<uint32_t>(QueueType::Copy)] = new DX12CommandQueue(DXQueueTypeFromQueueType(QueueType::Copy), "DX12CommandQueue::CopyQueue");
		m_queues[static_cast<uint32_t>(QueueType::Compute)] = new DX12CommandQueue(DXQueueTypeFromQueueType(QueueType::Compute), "DX12CommandQueue::ComputeQueue");

		// Allocate shadercompiler stuff.
		ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_utils)));
		ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_shaderCompiler)));


		// Allocate bindless rootsignature.
		D3D12_ROOT_PARAMETER1 parameters[1] = {};

		parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		parameters[0].Descriptor.ShaderRegister = 0;
		parameters[0].Constants.Num32BitValues = 64;
		parameters[0].Constants.RegisterSpace = 0;

		D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc;
		desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
		desc.Desc_1_1.NumParameters = _countof(parameters);
		desc.Desc_1_1.pParameters = parameters;
		desc.Desc_1_1.NumStaticSamplers = 0;
		desc.Desc_1_1.pStaticSamplers = 0;
		desc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED
			| D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;


		ComPtr<ID3DBlob> rootSignatureBlob;
		ComPtr<ID3DBlob> errorBlob;
		ThrowIfFailed(D3D12SerializeVersionedRootSignature(&desc, rootSignatureBlob.GetAddressOf(), errorBlob.GetAddressOf()));
		ThrowIfFailed(m_device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_bindlessRootSignature)));
	}

	DX12GraphicsAPI::~DX12GraphicsAPI()
	{
		Log::Warn("Shutting down Direct X 12 context.");

		for (uint32_t i = 0; i < static_cast<uint32_t>(QueueType::Num); i++)
		{
			m_queues[i]->WaitForFence();
		}

		FREE(m_descriptorAllocatorSrvCbvUav)
		FREE(m_descriptorAllocatorsRtv);
		FREE(m_descriptorAllocatorsDsv);
		FREE(m_descriptorAllocatorsSampler);

		COM_FREE(m_device);
		COM_FREE(m_factory);

		COM_FREE(m_bindlessRootSignature);

		COM_FREE(m_shaderCompiler);
		COM_FREE(m_utils);
	}
}