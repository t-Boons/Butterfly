#include "Renderer/D3D12/CommandQueue_DX12.hpp"
#include "Renderer/D3D12/CommandList_DX12.hpp"
#include "Renderer/D3D12/Fence_DX12.hpp"
#include "Renderer/D3D12/GraphicsAPI_DX12.hpp"

namespace Butterfly
{
	DX12CommandQueue::DX12CommandQueue(D3D12_COMMAND_LIST_TYPE type, const std::string& resourceTag)
		: m_type(type)
	{
		BF_PROFILE_EVENT();

		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = type;
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		ThrowIfFailed(DX12API()->Device()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_queue)));
		m_queue->SetName(Utils::StringToWString(resourceTag).c_str());

		Log::Info("DX12 command queue created: %s", resourceTag.c_str());
	}

	DX12CommandQueue::~DX12CommandQueue()
	{
		BF_PROFILE_EVENT();

		COM_FREE(m_queue)
	}

	void DX12CommandQueue::Execute(uint32_t numLists, ID3D12CommandList* const* lists) const
	{
		BF_PROFILE_EVENT();

		CheckMsg(numLists > 0, "DX12CommandQueue numLists is 0")
		CheckMsg(Utils::IsArrayPtrValid(numLists, lists), "DX12CommandQueue Lists provided are invalid")

		m_queue->ExecuteCommandLists(numLists, lists);
	}

	void DX12CommandQueue::Execute(DX12CommandList& list) const
	{
		BF_PROFILE_EVENT();

		ID3D12CommandList* l[] = { list.List() };
		m_queue->ExecuteCommandLists(1, l);
	}

	void DX12CommandQueue::WaitForFence() const
	{
		BF_PROFILE_EVENT();

		DX12Fence fence;
		fence.SignalAndWait(*this);
	}
}