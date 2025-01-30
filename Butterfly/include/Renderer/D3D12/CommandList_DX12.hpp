#pragma once
#include "Common_DX12.hpp"
#include "Renderer/CommandList.hpp"

namespace Butterfly
{
	class DX12CommandListCache;
	class DX12CommandList;

	class DX12CommandListHandle : private NonCopyable
	{
	public:
		DX12CommandList& List() { return m_list; }

		~DX12CommandListHandle();

	private:
		DX12CommandListHandle(DX12CommandList& list, DX12CommandListCache& cache, uint32_t index);

		friend class DX12CommandListCache;

		uint32_t m_listIndex;
		DX12CommandListCache& m_cache;
		DX12CommandList& m_list;
	};


	class DX12CommandListCache : private NonCopyable
	{
	public:
		DX12CommandListCache();
		~DX12CommandListCache();

		DX12CommandListHandle* GetNewList(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);

		friend class DX12CommandListHandle;

	private:
		std::unordered_map<uint32_t, DX12CommandList*> m_lists;
		std::unordered_map<uint32_t, bool> m_isListInFlight;

		uint32_t m_listsInFlight;
		uint32_t m_listIndex;
	};

	static inline DX12CommandListCache* g_CommandListCache = nullptr;
	inline DX12CommandListCache* CommandListCache()
	{
		if (!g_CommandListCache) g_CommandListCache = new DX12CommandListCache();
		return g_CommandListCache;
	}


	class ScopedGPUMarker;

	class DX12CommandList : public CommandList
	{
	public:
		DX12CommandList(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);
		~DX12CommandList();

		virtual void Reset() override;
		virtual void Close() override;
		virtual void Marker(const std::string& str) override;
		virtual void BeginGPUMarker(const std::string& str) override;
		virtual void EndGPUMarker() override;

		ID3D12GraphicsCommandList* List() const { return m_cmdList; }
		D3D12_COMMAND_LIST_TYPE Type() const { return m_dxType; }

	private:
		bool m_cmdListClosed;
		bool m_hasExecuted;
		bool m_eventInFlight;
		ID3D12GraphicsCommandList* m_cmdList;
		ID3D12CommandAllocator* m_allocator;
		D3D12_COMMAND_LIST_TYPE m_dxType;
	};
}