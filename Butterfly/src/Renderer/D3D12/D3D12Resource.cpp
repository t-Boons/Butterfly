#include "Renderer/D3D12/D3D12Resource.hpp"
#include "Renderer/D3D12/D3D12CommandList.hpp"
#include "Renderer/D3D12/D3D12GraphicsAPI.hpp"
#include "Renderer/D3D12/D3D12SubresourceRange.hpp"

namespace Butterfly
{
	namespace Utils
	{
		constexpr D3D12_RESOURCE_STATES WriteStates =
			D3D12_RESOURCE_STATE_RENDER_TARGET |
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS |
			D3D12_RESOURCE_STATE_DEPTH_WRITE |
			D3D12_RESOURCE_STATE_COPY_DEST |
			D3D12_RESOURCE_STATE_RESOLVE_DEST |
			D3D12_RESOURCE_STATE_STREAM_OUT;

		bool IsReadOnly(D3D12_RESOURCE_STATES state)
		{
			return state != D3D12_RESOURCE_STATE_COMMON && (state & WriteStates) == 0;
		}

		D3D12_RESOURCE_STATES ResolveState(D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES requested)
		{
			if (IsReadOnly(before) && IsReadOnly(requested))
			{
				return before | requested;
			}
			
			return requested;
		}
	}

		D3D12Resource* D3D12Resource::Write(const void* src, uint32_t numBytes, uint32_t offset)
		{
			std::stringstream ss;
			ss << "D3D12Resource::Write -> " << DebugName;
			BF_PROFILE_EVENT_DYNAMIC(ss.str().c_str());

			void* pData;
			HwResource->Map(0, nullptr, &pData);
			memcpy((char*)pData + offset, src, numBytes);
			HwResource->Unmap(0, nullptr);

			return this;
		}

		void* D3D12Resource::Map()
		{
			BF_PROFILE_EVENT();
			void* pData;
			HwResource->Map(0, nullptr, &pData);
			return pData;
		}

		void D3D12Resource::Unmap()
		{
			BF_PROFILE_EVENT();
			HwResource->Unmap(0, nullptr);
		}

		uint32_t D3D12Resource::MipCount() const
		{
			return BufferDescription.MipLevels;
		}

		uint32_t D3D12Resource::ArraySize() const
		{
			return BufferDescription.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? 1u :BufferDescription.DepthOrArraySize;
		}

		uint32_t D3D12Resource::NumSubresources() const
		{
			return BufferDescription.MipLevels * ArraySize();
		}

		void D3D12Resource::TransitionRange(const D3D12CommandList& cmdList, const D3D12_RESOURCE_STATES& newState, const D3D12SubresourceRange& range)
		{
			BF_PROFILE_EVENT();

			if(!IsTransitionable)
			{
				return;
			}

			if (range.IsAll())
			{
				TransitionAll(cmdList, newState);
				return;
			}

			const uint32_t mipCount = BufferDescription.MipLevels;
			const uint32_t sliceCount = ArraySize();

			const uint32_t mipEnd = (range.MipCount == BF_ALL) ? mipCount : range.FirstMip + range.MipCount;
			const uint32_t sliceEnd = (range.SliceCount == BF_ALL) ? sliceCount : range.FirstSlice + range.SliceCount;

			for (uint32_t slice = range.FirstSlice; slice < sliceEnd; ++slice)
			{
				for (uint32_t mip = range.FirstMip; mip < mipEnd; ++mip)
				{
					TransitionSubresource(cmdList, newState, mip, slice);
				}
			}
		}

		D3D12Resource* D3D12Resource::TransitionSubresource(const D3D12CommandList& cmdList, const D3D12_RESOURCE_STATES& newState, uint32_t mipslice, uint32_t arraySlice)
		{
			BF_PROFILE_EVENT();

			if (!IsTransitionable)
			{
				return this;
			}
			const uint32_t subresourceIndex = D3D12CalcSubresource(mipslice, arraySlice, 0, BufferDescription.MipLevels, BufferDescription.DepthOrArraySize);

			auto it = CurrentResourceStates.find(subresourceIndex);
			const D3D12_RESOURCE_STATES before = (it != CurrentResourceStates.end()) ? it->second : WholeState;
			const D3D12_RESOURCE_STATES after = Utils::ResolveState(before, newState);

			if (before == after)
			{
				if (after == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
				{
					D3D12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(HwResource);
					cmdList.List()->ResourceBarrier(1, &uavBarrier);
				}

				return this;
			}

			D3D12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(HwResource, before, after, subresourceIndex);
			cmdList.List()->ResourceBarrier(1, &transition);
			CurrentResourceStates[subresourceIndex] = after;
			return this;
		}

		D3D12Resource* D3D12Resource::TransitionAll(const D3D12CommandList& cmdList, const D3D12_RESOURCE_STATES& newState)
		{
			BF_PROFILE_EVENT();

			if (!IsTransitionable)
			{
				return this;
			}

			// The resource has not been divided into subresources.
			if (CurrentResourceStates.empty())
			{
				const D3D12_RESOURCE_STATES after = Utils::ResolveState(WholeState, newState);

				if (WholeState == after)
				{
					if (after == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
					{
						D3D12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(HwResource);
						cmdList.List()->ResourceBarrier(1, &uavBarrier);
					}
					return this;
				}

				D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(HwResource, WholeState, after);
				cmdList.List()->ResourceBarrier(1, &barrier);
				WholeState = after;
				return this;
			}

			std::vector<D3D12_RESOURCE_BARRIER> barriers;
			std::vector<D3D12_RESOURCE_STATES> afterStates(NumSubresources());
			bool needsUAVBarrier = false;

			for (uint32_t i = 0; i < NumSubresources(); ++i)
			{
				auto it = CurrentResourceStates.find(i);
				const D3D12_RESOURCE_STATES before = (it != CurrentResourceStates.end()) ? it->second : WholeState;
				const D3D12_RESOURCE_STATES after = Utils::ResolveState(before, newState);

				if (before != after)
				{
					barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(HwResource, before, after, i));
				}
				else if (after == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
				{
					needsUAVBarrier = true;
				}

				afterStates[i] = after;
			}

			if (needsUAVBarrier)
			{
				barriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(HwResource));
			}

			if (!barriers.empty())
			{
				cmdList.List()->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
			}

			// If everything ended up in the same state, go back to using the WholeState;
			CurrentResourceStates.clear();
			const bool allSame = std::all_of(afterStates.begin(), afterStates.end(), [&](D3D12_RESOURCE_STATES s) { return s == afterStates[0]; });
			if (allSame)
			{
				WholeState = afterStates[0];
			}
			else
			{
				for (uint32_t i = 0; i < NumSubresources(); ++i)
				{
					CurrentResourceStates[i] = afterStates[i];
				}
			}
			return this;
		}

		D3D12Resource::~D3D12Resource()
		{
			BF_PROFILE_EVENT();

			COM_FREE(HwResource);
		}

		D3D12Resource::D3D12Resource()
			: BufferDescription(D3D12_RESOURCE_DESC()), HeapProperties(CD3DX12_HEAP_PROPERTIES()), HwResource(nullptr),
			ClearValue(D3D12_CLEAR_VALUE()), DebugName("Resource")
		{
		}

		void D3D12Resource::Alloc()
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

			IsTransitionable = HeapProperties.Type != D3D12_HEAP_TYPE_UPLOAD && HeapProperties.Type != D3D12_HEAP_TYPE_READBACK;

			// Create the DX12 resource.
			ThrowIfFailed(D3D12API()->Device()->CreateCommittedResource(
				&HeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&BufferDescription,
				WholeState,
				clearValue,
				IID_PPV_ARGS(&HwResource)
			));

			BufferDescription = HwResource->GetDesc();

			// Set its debug name.
			HwResource->SetName(Utils::StringToWString(DebugName).c_str());
		}


		DX12ResourceBuilder::DX12ResourceBuilder()
			: m_hasBeenCreated(false)
		{
			m_resource = new D3D12Resource();
		}

		DX12ResourceBuilder::~DX12ResourceBuilder()
		{
			if (!m_hasBeenCreated)
			{
				BF_CORE_LOG_WARN("Resource freed by DX12ResourceBuilder. Are you sure you created the resource?");
				FREE(m_resource);
			}
		}

		D3D12Resource* DX12ResourceBuilder::CreateFromSwapchain(ID3D12Resource2* resource, D3D12_RESOURCE_STATES state)
		{
			BF_PROFILE_EVENT();

			m_hasBeenCreated = true;

			CD3DX12_HEAP_PROPERTIES props;
			resource->GetHeapProperties(&props, nullptr);
			m_resource->HeapProperties = props;
			m_resource->BufferDescription = resource->GetDesc();
			m_resource->HwResource = resource;
			m_resource->WholeState = state;
			m_resource->HwResource->SetName(Utils::StringToWString(m_resource->DebugName).c_str());
			return m_resource;
		}

		D3D12Resource* DX12ResourceBuilder::Create()
		{
			m_hasBeenCreated = true;
			m_resource->Alloc();

			return m_resource;
		}

		DX12ResourceBuilder& DX12ResourceBuilder::Flags(const D3D12_RESOURCE_FLAGS& flag)
		{
			m_resource->BufferDescription.Flags = flag;
			return *this;
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

		DX12ResourceBuilder& DX12ResourceBuilder::Texture(DXGI_FORMAT format, uint32_t width, uint32_t height, uint32_t arraySize, uint32_t numMips)
		{
			m_resource->BufferDescription = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, arraySize, numMips);
			return *this;
		}

		DX12ResourceBuilder& DX12ResourceBuilder::Texture3D(DXGI_FORMAT format, uint32_t width, uint32_t height, uint32_t depth, uint32_t numMips)
		{
			m_resource->BufferDescription = CD3DX12_RESOURCE_DESC::Tex3D(format, width, height, depth, numMips);
			return *this;
		}

		DX12ResourceBuilder& DX12ResourceBuilder::RenderTarget(DXGI_FORMAT format, uint32_t width, uint32_t height)
		{
			m_resource->BufferDescription = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height);
			m_resource->BufferDescription.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			return *this;
		}

		DX12ResourceBuilder& DX12ResourceBuilder::DepthStencil(DXGI_FORMAT format, uint32_t width, uint32_t height, uint32_t arraySize)
		{
			m_resource->BufferDescription = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, arraySize);
			m_resource->BufferDescription.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			return *this;
		}

		DX12ResourceBuilder& DX12ResourceBuilder::InitialState(const D3D12_RESOURCE_STATES& state)
		{
			m_resource->WholeState = state;
			return *this;
		}
}
