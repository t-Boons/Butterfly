#include "Renderer/D3D12/CommandList_DX12.hpp"
#include "Renderer/D3D12/GraphicsAPI_DX12.hpp"

namespace Butterfly
{
	DX12CommandListHandle::DX12CommandListHandle(DX12CommandList& list, DX12CommandListCache& cache, uint32_t index)
		: m_list(list), m_cache(cache), m_listIndex(index)
	{
		m_cache.m_isListInFlight[m_listIndex] = true;
		m_cache.m_listsInFlight++;
	}
	DX12CommandListHandle::~DX12CommandListHandle()
	{
		m_cache.m_isListInFlight[m_listIndex] = false;
		m_cache.m_listsInFlight--;
	}





	DX12CommandListCache::DX12CommandListCache()
		: m_listsInFlight(0), m_listIndex(0)
	{
	}

	DX12CommandListCache::~DX12CommandListCache()
	{
		BF_PROFILE_EVENT();

		CheckMsg(m_listsInFlight == 0, "DX12CommandListCache was destructed but it still has Lists in flight.");

		for (auto& list : m_lists)
		{
			FREE(list.second);
		}
	}


	DX12CommandListHandle* DX12CommandListCache::GetNewList(D3D12_COMMAND_LIST_TYPE type)
	{
		BF_PROFILE_EVENT();

		for (auto& list : m_lists)
		{
			// Check if the list has the same type and is in flight.
			if (list.second->Type() == type &&
				m_isListInFlight[list.first] == false)
			{
				//If so return a new handle for that list.
				return new DX12CommandListHandle(*list.second, *this, list.first);
			}
		}

		Log::Info("Created new DX12CommandList from cache with index: %u", m_listIndex);

		// Create a new list.
		m_listIndex++;
		m_lists[m_listIndex] = new DX12CommandList(type);

		return new DX12CommandListHandle(*m_lists[m_listIndex], *this, m_listIndex);
	}





	DX12CommandList::DX12CommandList(D3D12_COMMAND_LIST_TYPE type)
		: m_hasExecuted(false), m_cmdListClosed(false), m_eventInFlight(false)
	{
		BF_PROFILE_EVENT();

		ThrowIfFailed(DX12API()->Device()->CreateCommandAllocator(type, IID_PPV_ARGS(&m_allocator)));
		ThrowIfFailed(DX12API()->Device()->CreateCommandList(0, type, m_allocator, nullptr, IID_PPV_ARGS(&m_cmdList)));
	}

	DX12CommandList::~DX12CommandList()
	{
		BF_PROFILE_EVENT();

		COM_FREE(m_cmdList);
		COM_FREE(m_allocator);
	}

	void DX12CommandList::Reset()
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

	void DX12CommandList::Close()
	{
		BF_PROFILE_EVENT();

		if (!m_cmdListClosed)
		{
			m_cmdList->Close();
			m_cmdListClosed = true;
			return;
		}
	}

	void DX12CommandList::Marker(const std::string& str)
	{
		m_cmdList->SetMarker(1u, str.c_str(), static_cast<uint32_t>(str.size() + 1));
	}

	void DX12CommandList::BeginGPUMarker(const std::string& str)
	{
		if (m_eventInFlight)
		{
			Log::Warn("DX12CommandList::BeginGPUMarker is already running.");
			return;
		}

		m_eventInFlight = true;
		m_cmdList->BeginEvent(1u, str.c_str(), static_cast<uint32_t>(str.size() + 1));
	}

	void DX12CommandList::EndGPUMarker()
	{
		if (!m_eventInFlight)
		{
			Log::Warn("DX12CommandList::EndGPUMarker called on CommandList without running GPU marker.");
			return;
		}
		m_eventInFlight = false;
		m_cmdList->EndEvent();
	}
}