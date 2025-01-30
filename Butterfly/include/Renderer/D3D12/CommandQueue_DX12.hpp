#pragma once
#include "Common_DX12.hpp"

namespace Butterfly
{
	class DX12CommandList;

	class DX12CommandQueue
	{
	public:
		DX12CommandQueue(D3D12_COMMAND_LIST_TYPE type, const std::string& resourceTag);
		~DX12CommandQueue();

		void Execute(uint32_t numLists, ID3D12CommandList* const* lists) const;
		void Execute(DX12CommandList& list) const;

		void WaitForFence() const;
		ID3D12CommandQueue* D3D12Queue() const { return m_queue; }

		D3D12_COMMAND_LIST_TYPE Type() const { return m_type; }

	private:
		ID3D12CommandQueue* m_queue;
		D3D12_COMMAND_LIST_TYPE m_type;
	};
}

