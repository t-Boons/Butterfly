#include "Renderer/D3D12/Resource_DX12.hpp"
#include "Renderer/D3D12/CommandList_DX12.hpp"
#include "Renderer/D3D12/GraphicsAPI_DX12.hpp"

namespace Butterfly
{
		DX12Resource* DX12Resource::Write(const void* src, uint32_t numBytes)
		{
			std::stringstream ss;
			ss << "DX12Resource::Write -> " << DebugName;
			BF_PROFILE_EVENT_DYNAMIC(ss.str().c_str());


			void* pData;
			HwResource->Map(0, nullptr, &pData);
			memcpy(pData, src, numBytes);
			HwResource->Unmap(0, nullptr);

			return this;
		}

		DX12Resource* DX12Resource::Transition(const DX12CommandList& cmdList, const D3D12_RESOURCE_STATES& newState)
		{
			BF_PROFILE_EVENT();


			if (CurrentResourceState == newState)
			{
				return this;
			}

			D3D12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(HwResource, CurrentResourceState, newState);
			cmdList.List()->ResourceBarrier(1, &transition);
			CurrentResourceState = newState;
			return this;
		}

		DX12Resource::~DX12Resource()
		{
			BF_PROFILE_EVENT();

			COM_FREE(HwResource);
		}

		DX12Resource::DX12Resource()
			: BufferDescription(D3D12_RESOURCE_DESC()), HeapProperties(CD3DX12_HEAP_PROPERTIES()), HwResource(nullptr),
			ClearValue(D3D12_CLEAR_VALUE()), DebugName("Resource"), CurrentResourceState(D3D12_RESOURCE_STATE_GENERIC_READ)
		{
		}

		void DX12Resource::Alloc()
		{
			std::stringstream ss;
			ss << "DX12Resource::Alloc -> " << DebugName;
			BF_PROFILE_EVENT_DYNAMIC(ss.str().c_str());

			// Get the right clearvalue if any depending on the resource type.
			D3D12_CLEAR_VALUE* clearValue = &ClearValue;

			if (BufferDescription.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
			{
				clearValue = nullptr;
			}

			if (!(BufferDescription.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET ||
				BufferDescription.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL))
			{
				clearValue = nullptr;
			}

			// Create the DX12 resource.
			ThrowIfFailed(DX12API()->Device()->CreateCommittedResource(
				&HeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&BufferDescription,
				CurrentResourceState,
				clearValue,
				IID_PPV_ARGS(&HwResource)
			));

			// Set its debug name.
			HwResource->SetName(Utils::StringToWString(DebugName).c_str());
		}


		DX12ResourceBuilder::DX12ResourceBuilder()
			: m_hasBeenCreated(false)
		{
			m_resource = new DX12Resource();
		}

		DX12ResourceBuilder::~DX12ResourceBuilder()
		{
			if (!m_hasBeenCreated)
			{
				Log::Warn("Resource freed by DX12ResourceBuilder. Are you sure you created the resource?");
				FREE(m_resource);
			}
		}

		DX12Resource* DX12ResourceBuilder::CreateFromSwapchain(ID3D12Resource2* resource, D3D12_RESOURCE_STATES state)
		{
			BF_PROFILE_EVENT();

			m_hasBeenCreated = true;

			CD3DX12_HEAP_PROPERTIES props;
			resource->GetHeapProperties(&props, nullptr);
			m_resource->HeapProperties = props;
			m_resource->BufferDescription = resource->GetDesc();
			m_resource->HwResource = resource;
			m_resource->CurrentResourceState = state;
			m_resource->HwResource->SetName(Utils::StringToWString(m_resource->DebugName).c_str());
			return m_resource;
		}

		DX12Resource* DX12ResourceBuilder::Create()
		{
			m_hasBeenCreated = true;
			m_resource->Alloc();

			return m_resource;
		}

		DX12ResourceBuilder& DX12ResourceBuilder::SetName(const std::string& name)
		{
			m_resource->DebugName = name;
			return *this;
		}

		DX12ResourceBuilder& DX12ResourceBuilder::HeapType(D3D12_HEAP_TYPE type)
		{
			m_resource->HeapProperties = CD3DX12_HEAP_PROPERTIES(type);
			return *this;
		}

		DX12ResourceBuilder& DX12ResourceBuilder::ClearColor(float* color, DXGI_FORMAT format)
		{
			for (size_t i = 0; i < 4; i++)
				m_resource->ClearValue.Color[i] = color[i];

			m_resource->ClearValue.Format = format;
			return *this;
		}

		DX12ResourceBuilder& DX12ResourceBuilder::ClearDepth(DXGI_FORMAT format)
		{
			BF_PROFILE_EVENT();

			m_resource->ClearValue.Format = format;
			m_resource->ClearValue.DepthStencil.Depth = 1.0f;
			m_resource->ClearValue.DepthStencil.Stencil = 0u;
			return *this;
		}

		DX12ResourceBuilder& DX12ResourceBuilder::Buffer(const size_t& numBytes)
		{
			m_resource->BufferDescription = CD3DX12_RESOURCE_DESC::Buffer(numBytes);
			return *this;
		}

		DX12ResourceBuilder& DX12ResourceBuilder::IndexBuffer(const size_t& numBytes)
		{
			m_resource->BufferDescription = CD3DX12_RESOURCE_DESC::Buffer(numBytes);
			m_resource->BufferDescription.Flags = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
			return *this;
		}

		DX12ResourceBuilder& DX12ResourceBuilder::Texture2D(DXGI_FORMAT format, uint32_t width, uint32_t height)
		{
			m_resource->BufferDescription = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height);
			return *this;
		}

		DX12ResourceBuilder& DX12ResourceBuilder::RenderTarget(DXGI_FORMAT format, uint32_t width, uint32_t height)
		{
			m_resource->BufferDescription = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height);
			m_resource->BufferDescription.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			return *this;
		}

		DX12ResourceBuilder& DX12ResourceBuilder::DepthStencil(DXGI_FORMAT format, uint32_t width, uint32_t height)
		{
			m_resource->BufferDescription = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height);
			m_resource->BufferDescription.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			return *this;
		}

		DX12ResourceBuilder& DX12ResourceBuilder::InitialState(const D3D12_RESOURCE_STATES& state)
		{
			m_resource->CurrentResourceState = state;
			return *this;
		}
}