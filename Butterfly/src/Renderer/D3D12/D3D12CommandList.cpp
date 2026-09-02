#include "Renderer/D3D12/D3D12CommandList.hpp"
#include "Renderer/D3D12/D3D12GraphicsAPI.hpp"

namespace Butterfly
{
	D3D12CommandList::D3D12CommandList(D3D12_COMMAND_LIST_TYPE type)
		: m_hasExecuted(false), m_cmdListClosed(false), m_eventInFlight(false)
	{
		BF_PROFILE_EVENT();

		ThrowIfFailed(D3D12API()->Device()->CreateCommandAllocator(type, IID_PPV_ARGS(&m_allocator)));
		ThrowIfFailed(D3D12API()->Device()->CreateCommandList(0, type, m_allocator, nullptr, IID_PPV_ARGS(&m_cmdList)));
	}

	D3D12CommandList::~D3D12CommandList()
	{
		BF_PROFILE_EVENT();

		COM_FREE(m_cmdList);
		COM_FREE(m_allocator);
	}

	void D3D12CommandList::Reset()
	{
		BF_PROFILE_EVENT();

		if (!m_cmdListClosed)
		{
			m_cmdList->Close();
		}

		m_hasExecuted = false;
		m_allocator->Reset();
		m_cmdList->Reset(m_allocator, nullptr);

		m_cmdListClosed = false;
	}

	void D3D12CommandList::Close()
	{
		BF_PROFILE_EVENT();

		if (!m_cmdListClosed)
		{
			m_cmdList->Close();
			m_cmdListClosed = true;
			return;
		}
	}

	void D3D12CommandList::Marker(const std::string& str)
	{
		m_cmdList->SetMarker(1u, str.c_str(), static_cast<uint32_t>(str.size() + 1));
	}

	void D3D12CommandList::BeginGPUMarker(const std::string& str)
	{
		if (m_eventInFlight)
		{
			BF_CORE_LOG_WARN("DX12CommandList::BeginGPUMarker is already running.");
			return;
		}

		m_eventInFlight = true;
		m_cmdList->BeginEvent(1u, str.c_str(), static_cast<uint32_t>(str.size() + 1));
	}

	void D3D12CommandList::EndGPUMarker()
	{
		if (!m_eventInFlight)
		{
			BF_CORE_LOG_WARN("DX12CommandList::EndGPUMarker called on CommandList without running GPU marker.");
			return;
		}
		m_eventInFlight = false;
		m_cmdList->EndEvent();
	}
}
