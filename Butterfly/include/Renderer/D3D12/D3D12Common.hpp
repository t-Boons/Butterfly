#pragma once
#include "Core/Common.hpp"

#include <dxgi1_6.h>
#include <windows.h>
#include <wrl.h>
#include "d3dx12/d3dx12.h"
#include "d3d12.h"
#include "dxc/include/dxcapi.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")

using namespace Microsoft::WRL;

#define COM_FREE(p)     \
    {if (p) {            \
        p->Release();   \
        p = nullptr;    \
    }}

namespace Butterfly
{
	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
            char* hrCstr = nullptr;
            FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr,
                hr,
                0,
                (LPSTR)&hrCstr,
                0,
                nullptr
            );
			Log::Critical("HRESULT Error: %s", hrCstr);
		}
	}

    struct PipelineStateStream
    {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
        CD3DX12_PIPELINE_STATE_STREAM_VS VS;
        CD3DX12_PIPELINE_STATE_STREAM_PS PS;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
        CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER RasterizerState;
        CD3DX12_PIPELINE_STATE_STREAM_STREAM_OUTPUT StreamOutput;
        CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC BlendDesc;
    };
}