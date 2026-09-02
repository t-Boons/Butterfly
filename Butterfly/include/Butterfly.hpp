#pragma once

// API
#include "Renderer/GraphicsAPI.hpp"
#include "Renderer/D3D12/GraphicsAPI_D3D12.hpp"

// Platform agnostic types.
#include "Renderer/CommandList.hpp"

// Core types.
#include "Core/Window.hpp"

#include "Renderer/D3D12/DescriptorAllocator_D3D12.hpp"
#include "Renderer/D3D12/CommandQueue_D3D12.hpp"
#include "Renderer/D3D12/CommandList_D3D12.hpp"
#include "Renderer/D3D12/Context_D3D12.hpp"
#include "Renderer/D3D12/Fence_D3D12.hpp"
#include "Renderer/D3D12/View_D3D12.hpp"
#include "Renderer/D3D12/Pipeline_D3D12.hpp"
#include "Renderer/D3D12/GraphicsCommands_D3D12.hpp"
#include "Renderer/D3D12/Resource_D3D12.hpp"
#include "Renderer/D3D12/Shader_D3D12.hpp"
#include "Renderer/D3D12/ShaderVariables_D3D12.hpp"

// Model loading.
#include "Renderer/ModelLoading/ModelImporter.hpp"

// Butterfly types.
#include "Renderer/Sampler_D3D12.hpp"
#include "Renderer/Buffer_D3D12.hpp"
#include "Renderer/Texture_D3D12.hpp"

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