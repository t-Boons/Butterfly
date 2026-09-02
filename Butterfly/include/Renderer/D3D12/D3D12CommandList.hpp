#pragma once
#include "D3D12Common.hpp"
#include "Renderer/CommandList.hpp"

namespace Butterfly
{
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
