#pragma once
#include "Common_DX12.hpp"
#include "Renderer/Types.hpp"

namespace Butterfly
{
	inline D3D12_COMMAND_LIST_TYPE DXQueueTypeFromQueueType(QueueType type)
	{
		switch (type)
		{
		case QueueType::Direct: return D3D12_COMMAND_LIST_TYPE_DIRECT;
		case QueueType::Copy: return D3D12_COMMAND_LIST_TYPE_COPY;
		case QueueType::Compute: return D3D12_COMMAND_LIST_TYPE_COMPUTE;
		default: Check(false); return static_cast<D3D12_COMMAND_LIST_TYPE>(-1);
		}
	}
}