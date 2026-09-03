#include "Renderer/D3D12/D3D12CommandQueue.hpp"
#include "Renderer/D3D12/D3D12CommandList.hpp"
#include "Renderer/D3D12/D3D12Fence.hpp"
#include "Renderer/D3D12/D3D12GraphicsAPI.hpp"

namespace Butterfly
{
	D3D12CommandQueue::D3D12CommandQueue(D3D12_COMMAND_LIST_TYPE type, const std::string& resourceTag)
		: m_type(type)
	{
		BF_PROFILE_EVENT();

		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = type;
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		ThrowIfFailed(D3D12API()->Device()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_queue)));
		m_queue->SetName(Utils::StringToWString(resourceTag).c_str());

		BF_CORE_LOG_INFO("DX12 command queue created: %s", resourceTag.c_str());
	}

	D3D12CommandQueue::~D3D12CommandQueue()
	{
		BF_PROFILE_EVENT();

		COM_FREE(m_queue)
	}

	void D3D12CommandQueue::Execute(uint32_t numLists, ID3D12CommandList* const* lists) const
	{
		BF_PROFILE_EVENT();

		BF_CORE_ASSERT(numLists > 0, "D3D12CommandQueue numLists is 0");
		BF_CORE_ASSERT(Utils::IsArrayPtrValid(numLists, lists), "D3D12CommandQueue Lists provided are invalid");

		m_queue->ExecuteCommandLists(numLists, lists);
	}

	void D3D12CommandQueue::Execute(D3D12CommandList& list) const
	{
		BF_PROFILE_EVENT();

		ID3D12CommandList* l[] = { list.List() };
		m_queue->ExecuteCommandLists(1, l);
	}

	void D3D12CommandQueue::WaitForFence() const
	{
		BF_PROFILE_EVENT();

		D3D12Fence fence;
		fence.SignalAndWait(*this);
	}
}
