#include "Renderer/GraphicsAPI.hpp"
#include "Renderer/D3D12/GraphicsAPI_DX12.hpp"

namespace Butterfly
{
	GraphicsAPI* Graphics = nullptr;

	void GraphicsAPI::Init(GraphicsAPIType api, bool enableDebug)
	{
		switch (api)
		{
		case GraphicsAPIType::D3D12: Graphics = new DX12GraphicsAPI(); break;
		default: Throw("No valid GraphicsAPI.")
		}
		Graphics->_Init(enableDebug);
	}
}