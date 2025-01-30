#pragma once
#include "Common_DX12.hpp"

namespace Butterfly
{
	class DX12CommandQueue;

	class DX12Fence : private NonCopyable
	{
	public:
		DX12Fence();
		~DX12Fence();

		void SignalAndWait(const DX12CommandQueue& queue);

		void Signal(const DX12CommandQueue& queue);
		void Wait();

	private:

		ID3D12Fence* m_fence;
		uint64_t m_fenceValue;
		HANDLE m_fenceEvent;
	};
}