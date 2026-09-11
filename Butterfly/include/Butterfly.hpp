#pragma once

// API
#include "Renderer/GraphicsAPI.hpp"
#include "Renderer/D3D12/D3D12GraphicsAPI.hpp"

// Platform agnostic types.
#include "Renderer/CommandList.hpp"

// Core types.
#include "Core//Log.hpp"
#include "Core/Window.hpp"
#include "Core/Application.hpp"
#include "Core/Time.hpp"
#include "Core/FileSystem.hpp"

// Asset types
#include "Asset/AssetManager.hpp"
#include "Asset/AssetRegistry.hpp"
#include "Asset/AssetTypes.hpp"

// Scene and Registry
#include "Scene/Scene.hpp"
#include "Scene/Registry/TransformComponent.hpp"
#include "Scene/Registry/MeshRendererComponent.hpp"
#include "Scene/Registry/NameComponent.hpp"

#include "Renderer/Renderer.hpp" 
#include "Renderer/D3D12/D3D12DescriptorAllocator.hpp"
#include "Renderer/D3D12/D3D12CommandQueue.hpp"
#include "Renderer/D3D12/D3D12CommandList.hpp"
#include "Renderer/D3D12/D3D12Context.hpp"
#include "Renderer/D3D12/D3D12Fence.hpp"
#include "Renderer/D3D12/D3D12View.hpp"
#include "Renderer/D3D12/D3D12Pipeline.hpp"
#include "Renderer/D3D12/D3D12GraphicsCommands.hpp"
#include "Renderer/D3D12/D3D12Resource.hpp"
#include "Renderer/D3D12/D3D12Shader.hpp"
#include "Renderer/D3D12/D3D12ShaderVariables.hpp"

// Model loading.
#include "Renderer/ModelLoading/ModelImporter.hpp"

// Butterfly types.
#include "Renderer/D3D12Sampler.hpp"
#include "Renderer/D3D12Buffer.hpp"
#include "Renderer/D3D12Texture.hpp"
#include "Renderer/D3D12/D3D12Fence.hpp"

// Butterfly (Render)Graph
#include "Renderer/Graph/GraphBuilder.hpp"
#include "Renderer/Graph/Graph.hpp"
#include "Renderer/Graph/Blackboard.hpp"
#include "Renderer/Graph/GraphTransientResourceCache.hpp"

// Utils.
#include "Utils/FullscreenQuad.hpp"
#include "Utils/Utils.hpp"
#include "Utils/Profiler.hpp"
