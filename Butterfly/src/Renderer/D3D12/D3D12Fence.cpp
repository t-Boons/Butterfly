#include "Renderer/D3D12/D3D12Fence.hpp"
#include "Renderer/D3D12/D3D12CommandQueue.hpp"
#include "Renderer/D3D12/D3D12GraphicsAPI.hpp"

namespace Butterfly
{
	D3D12Fence::D3D12Fence()
		: m_fenceValue(0)
	{
		BF_PROFILE_EVENT();

		ThrowIfFailed(D3D12API()->Device()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		BF_CORE_ASSERT(m_fenceEvent, "Failed to create fence event.");
	}

	D3D12Fence::~D3D12Fence()
	{
		BF_PROFILE_EVENT();
		COM_FREE(m_fence)
		CloseHandle(m_fenceEvent);
	}

	void D3D12Fence::SignalAndWait(const D3D12CommandQueue& queue)
	{
		BF_PROFILE_EVENT();

		Signal(queue);
		Wait();
	}

	void D3D12Fence::Signal(const D3D12CommandQueue& queue)
	{
		BF_PROFILE_EVENT();
		m_fenceValue++;
		ThrowIfFailed(queue.D3D12Queue()->Signal(m_fence, m_fenceValue));
	}

	void D3D12Fence::Wait()
	{
		BF_PROFILE_EVENT();
		if (m_fence->GetCompletedValue() < m_fenceValue)
		{
			m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent);
			WaitForSingleObject(m_fenceEvent, INFINITE);
		}
	}
}
