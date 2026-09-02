#pragma once
#include "D3D12Common.hpp"
#include "Renderer/CommandList.hpp"

namespace Butterfly
{
	class D3D12CommandListCache;
	class D3D12CommandList;

	class D3D12CommandListHandle : private NonCopyable
	{
	public:
		D3D12CommandList& List() { return m_list; }

		~D3D12CommandListHandle();

	private:
		D3D12CommandListHandle(D3D12CommandList& list, D3D12CommandListCache& cache, uint32_t index);

		friend class D3D12CommandListCache;

		uint32_t m_listIndex;
		D3D12CommandListCache& m_cache;
		D3D12CommandList& m_list;
	};


	class D3D12CommandListCache : private NonCopyable
	{
	public:
		D3D12CommandListCache();
		~D3D12CommandListCache();

		D3D12CommandListHandle* GetNewList(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);

		friend class D3D12CommandListHandle;

	private:
		std::unordered_map<uint32_t, D3D12CommandList*> m_lists;
		std::unordered_map<uint32_t, bool> m_isListInFlight;

		uint32_t m_listsInFlight;
		uint32_t m_listIndex;
	};

	static inline D3D12CommandListCache* g_CommandListCache = nullptr;
	inline D3D12CommandListCache* CommandListCache()
	{
		if (!g_CommandListCache) g_CommandListCache = new D3D12CommandListCache();
		return g_CommandListCache;
	}


	class D3D12CommandList : public CommandList
	{
	public:
		D3D12CommandList(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);
		~D3D12CommandList();

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
