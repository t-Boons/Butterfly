#include "Renderer/D3D12/Fence_DX12.hpp"
#include "Renderer/D3D12/CommandQueue_DX12.hpp"
#include "Renderer/D3D12/GraphicsAPI_DX12.hpp"

namespace Butterfly
{
	DX12Fence::DX12Fence()
		: m_fenceValue(0)
	{
		BF_PROFILE_EVENT();

		ThrowIfFailed(DX12API()->Device()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		Check(m_fenceEvent);
	}

	DX12Fence::~DX12Fence()
	{
		BF_PROFILE_EVENT();
		COM_FREE(m_fence)
		CloseHandle(m_fenceEvent);
	}

	void DX12Fence::SignalAndWait(const DX12CommandQueue& queue)
	{
		BF_PROFILE_EVENT();

		Signal(queue);
		Wait();
	}

	void DX12Fence::Signal(const DX12CommandQueue& queue)
	{
		BF_PROFILE_EVENT();
		m_fenceValue++;
		ThrowIfFailed(queue.D3D12Queue()->Signal(m_fence, m_fenceValue));
	}

	void DX12Fence::Wait()
	{
		BF_PROFILE_EVENT();
		if (m_fence->GetCompletedValue() < m_fenceValue)
		{
			m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent);
			WaitForSingleObject(m_fenceEvent, INFINITE);
		}
	}
}