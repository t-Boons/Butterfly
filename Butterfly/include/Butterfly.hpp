#pragma once

// API
#include "Renderer/GraphicsAPI.hpp"
#include "Renderer/D3D12/GraphicsAPI_DX12.hpp"

// Platform agnostic types.
#include "Renderer/CommandList.hpp"

// Core types.
#include "Core/Window.hpp"

#include "Renderer/D3D12/DescriptorAllocator_DX12.hpp"
#include "Renderer/D3D12/CommandQueue_DX12.hpp"
#include "Renderer/D3D12/CommandList_DX12.hpp"
#include "Renderer/D3D12/Context_DX12.hpp"
#include "Renderer/D3D12/Fence_DX12.hpp"
#include "Renderer/D3D12/View_DX12.hpp"
#include "Renderer/D3D12/Pipeline_DX12.hpp"
#include "Renderer/D3D12/GraphicsCommands_DX12.hpp"
#include "Renderer/D3D12/Resource_DX12.hpp"
#include "Renderer/D3D12/Shader_DX12.hpp"
#include "Renderer/D3D12/ShaderVariables_DX12.hpp"

// Model loading.
#include "Renderer/ModelLoading/ModelImporter.hpp"

// Butterfly types.
#include "Renderer/Sampler_DX12.hpp"
#include "Renderer/Buffer_DX12.hpp"
#include "Renderer/Texture_DX12.hpp"

// Butterfly (Render)Graph
#include "Renderer/Graph/GraphBuilder.hpp"
#include "Renderer/Graph/Graph.hpp"
#include "Renderer/Graph/Blackboard.hpp"
#include "Renderer/Graph/GraphTransientResourceCache.hpp"

// Utils.
#include "Utils/FullscreenQuad.hpp"
#include "Utils/Log.hpp"
#include "Utils/Utils.hpp"
#include "Utils/Profiler.hpp"