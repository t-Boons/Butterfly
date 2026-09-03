#pragma once
#include "D3D12Common.hpp"

namespace Butterfly
{
	class D3D12CommandQueue;

	class D3D12Fence : private NonCopyable
	{
	public:
		D3D12Fence();
		~D3D12Fence();

		void SignalAndWait(const D3D12CommandQueue& queue);

		void Signal(const D3D12CommandQueue& queue);
		void Wait();

		uint64_t FenceValue() const { return m_fenceValue; }
	private:

		ID3D12Fence* m_fence;
		uint64_t m_fenceValue;
		HANDLE m_fenceEvent;
	};
}
