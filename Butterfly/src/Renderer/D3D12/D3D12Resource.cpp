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

		D3D12Resource::D3D12Resource(const D3D12ResourceDescription& desc)
			: m_clearValue(desc.ClearValue),
			  m_bufferDescription(desc.BufferDescription),
			  m_heapProperties(desc.HeapProperties),
			  m_wholeState(desc.WholeState),
			  m_debugName(desc.DebugName),
			  m_hwResource(desc.ResourceReference)
		{
			std::stringstream ss;
			ss << "D3D12Resource::D3D12Resource -> " << m_debugName;
			BF_PROFILE_EVENT_DYNAMIC(ss.str().c_str());

			// Get the right clearvalue if any depending on the resource type.
			D3D12_CLEAR_VALUE* clearValue = &m_clearValue;

			if (m_bufferDescription.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
			{
				clearValue = nullptr;
			}

			if (!(m_bufferDescription.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET ||
				m_bufferDescription.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL))
			{
				clearValue = nullptr;
			}

			m_isTransitionable = m_heapProperties.Type != D3D12_HEAP_TYPE_UPLOAD && m_heapProperties.Type != D3D12_HEAP_TYPE_READBACK;

			if(!desc.ResourceReference)
			{
				ThrowIfFailed(D3D12API()->Device()->CreateCommittedResource(
					&m_heapProperties,
					D3D12_HEAP_FLAG_NONE,
					&m_bufferDescription,
					m_wholeState,
					clearValue,
					IID_PPV_ARGS(&m_hwResource)
				));
			}
			else
			{
				m_hwResource = desc.ResourceReference;
			}

			m_bufferDescription = m_hwResource->GetDesc();

			// Set its debug name.
			m_hwResource->SetName(Utils::StringToWString(m_debugName).c_str());
		}

		D3D12Resource* D3D12Resource::Write(const void* src, uint32_t numBytes, uint32_t offset)
		{
			std::stringstream ss;
			ss << "D3D12Resource::Write -> " << m_debugName;
			BF_PROFILE_EVENT_DYNAMIC(ss.str().c_str());

			void* pData;
			m_hwResource->Map(0, nullptr, &pData);
			memcpy((char*)pData + offset, src, numBytes);
			m_hwResource->Unmap(0, nullptr);

			return this;
		}

		void* D3D12Resource::Map()
		{
			BF_PROFILE_EVENT();
			void* pData;
			m_hwResource->Map(0, nullptr, &pData);
			return pData;
		}

		void D3D12Resource::Unmap()
		{
			BF_PROFILE_EVENT();
			m_hwResource->Unmap(0, nullptr);
		}

		uint32_t D3D12Resource::MipCount() const
		{
			return m_bufferDescription.MipLevels;
		}

		uint32_t D3D12Resource::ArraySize() const
		{
			return m_bufferDescription.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? 1u :m_bufferDescription.DepthOrArraySize;
		}

		uint32_t D3D12Resource::NumSubresources() const
		{
			return m_bufferDescription.MipLevels * ArraySize();
		}

		void D3D12Resource::TransitionRange(const D3D12CommandList& cmdList, const D3D12_RESOURCE_STATES& newState, const D3D12SubresourceRange& range)
		{
			BF_PROFILE_EVENT();

			if(!m_isTransitionable)
			{
				return;
			}

			if (range.IsAll())
			{
				TransitionAll(cmdList, newState);
				return;
			}

			const uint32_t mipCount = m_bufferDescription.MipLevels;
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

			if (!m_isTransitionable)
			{
				return this;
			}
			const uint32_t subresourceIndex = D3D12CalcSubresource(mipslice, arraySlice, 0, m_bufferDescription.MipLevels, m_bufferDescription.DepthOrArraySize);

			auto it = m_currentResourceStates.find(subresourceIndex);
			const D3D12_RESOURCE_STATES before = (it != m_currentResourceStates.end()) ? it->second : m_wholeState;
			const D3D12_RESOURCE_STATES after = Utils::ResolveState(before, newState);

			if (before == after)
			{
				if (after == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
				{
					D3D12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(m_hwResource);
					cmdList.List()->ResourceBarrier(1, &uavBarrier);
				}

				return this;
			}

			D3D12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(m_hwResource, before, after, subresourceIndex);
			cmdList.List()->ResourceBarrier(1, &transition);
			m_currentResourceStates[subresourceIndex] = after;
			return this;
		}

		D3D12Resource* D3D12Resource::TransitionAll(const D3D12CommandList& cmdList, const D3D12_RESOURCE_STATES& newState)
		{
			BF_PROFILE_EVENT();

			if (!m_isTransitionable)
			{
				return this;
			}

			// The resource has not been divided into subresources.
			if (m_currentResourceStates.empty())
			{
				const D3D12_RESOURCE_STATES after = Utils::ResolveState(m_wholeState, newState);

				if (m_wholeState == after)
				{
					if (after == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
					{
						D3D12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(m_hwResource);
						cmdList.List()->ResourceBarrier(1, &uavBarrier);
					}
					return this;
				}

				D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_hwResource, m_wholeState, after);
				cmdList.List()->ResourceBarrier(1, &barrier);
				m_wholeState = after;
				return this;
			}

			std::vector<D3D12_RESOURCE_BARRIER> barriers;
			std::vector<D3D12_RESOURCE_STATES> afterStates(NumSubresources());
			bool needsUAVBarrier = false;

			for (uint32_t i = 0; i < NumSubresources(); ++i)
			{
				auto it = m_currentResourceStates.find(i);
				const D3D12_RESOURCE_STATES before = (it != m_currentResourceStates.end()) ? it->second : m_wholeState;
				const D3D12_RESOURCE_STATES after = Utils::ResolveState(before, newState);

				if (before != after)
				{
					barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(m_hwResource, before, after, i));
				}
				else if (after == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
				{
					needsUAVBarrier = true;
				}

				afterStates[i] = after;
			}

			if (needsUAVBarrier)
			{
				barriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(m_hwResource));
			}

			if (!barriers.empty())
			{
				cmdList.List()->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
			}

			// If everything ended up in the same state, go back to using the WholeState;
			m_currentResourceStates.clear();
			const bool allSame = std::all_of(afterStates.begin(), afterStates.end(), [&](D3D12_RESOURCE_STATES s) { return s == afterStates[0]; });
			if (allSame)
			{
				m_wholeState = afterStates[0];
			}
			else
			{
				for (uint32_t i = 0; i < NumSubresources(); ++i)
				{
					m_currentResourceStates[i] = afterStates[i];
				}
			}
			return this;
		}

		D3D12Resource::~D3D12Resource()
		{
			BF_PROFILE_EVENT();

			COM_FREE(m_hwResource);
		}


		D3D12ResourceDescription& D3D12ResourceDescription::FromSwapchain(ID3D12Resource2* resource, D3D12_RESOURCE_STATES state)
		{
			BF_PROFILE_EVENT();
			CD3DX12_HEAP_PROPERTIES props;
			resource->GetHeapProperties(&props, nullptr);
			HeapProperties = props;
			BufferDescription = resource->GetDesc();
			ResourceReference = resource;
			WholeState = state;

			return *this;
		}

		D3D12ResourceDescription D3D12ResourceDescription::Buffer(uint32_t numBytes)
		{
			D3D12ResourceDescription desc;
			desc.BufferDescription = CD3DX12_RESOURCE_DESC::Buffer(static_cast<uint64_t>(numBytes));
			return desc;
		}

		D3D12ResourceDescription D3D12ResourceDescription::Texture(DXGI_FORMAT format, uint32_t width, uint32_t height, uint32_t arraySize, uint32_t numMips)
		{
			D3D12ResourceDescription desc;
			desc.BufferDescription = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, arraySize, numMips);
			return desc;
		}

		D3D12ResourceDescription D3D12ResourceDescription::Texture3D(DXGI_FORMAT format, uint32_t width, uint32_t height, uint32_t depth, uint32_t numMips)
		{
			D3D12ResourceDescription desc;
			desc.BufferDescription = CD3DX12_RESOURCE_DESC::Tex3D(format, width, height, depth, numMips);
			return desc;
		}

		D3D12ResourceDescription& D3D12ResourceDescription::InitialState(const D3D12_RESOURCE_STATES& state)
		{
			WholeState = state;
			return *this;
		}

		D3D12ResourceDescription& D3D12ResourceDescription::Flags(const D3D12_RESOURCE_FLAGS& flag)
		{
			BufferDescription.Flags = flag;
			return *this;
		}

		D3D12ResourceDescription& D3D12ResourceDescription::SetName(const std::string& name)
		{
			DebugName = name;
			return *this;
		}

		D3D12ResourceDescription& D3D12ResourceDescription::HeapType(D3D12_HEAP_TYPE type)
		{
			HeapProperties = CD3DX12_HEAP_PROPERTIES(type);
			return *this;
		}

		D3D12ResourceDescription& D3D12ResourceDescription::ClearColor(float* color, DXGI_FORMAT format)
		{
			for (size_t i = 0; i < 4; i++)
			{
				ClearValue.Color[i] = color[i];
			}

			ClearValue.Format = format;
			return *this;
		}

		D3D12ResourceDescription& D3D12ResourceDescription::ClearDepth(DXGI_FORMAT format)
		{
			BF_PROFILE_EVENT();

			ClearValue.Format = format;
			ClearValue.DepthStencil.Depth = 1.0f;
			ClearValue.DepthStencil.Stencil = 0u;
			return *this;
		}
}
