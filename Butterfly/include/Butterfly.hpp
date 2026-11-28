#pragma once

// API
#include "Renderer/GraphicsAPI.hpp"
#include "Renderer/D3D12/D3D12GraphicsAPI.hpp"

// Platform agnostic types.
#include "Renderer/CommandList.hpp"

// Core types.
#include "Core/Common.hpp"
#include "Core/Window.hpp"
#include "Core/Application.hpp"
#include "Core/Time.hpp"
#include "Core/FileSystem.hpp"
#include "Core/JobSystem.hpp"
#include "Input/Input.hpp"

// Asset types
#include "Asset/AssetManager.hpp"
#include "Asset/AssetRegistry.hpp"
#include "Asset/AssetTypes.hpp"

// Level and Registry
#include "Level/Level.hpp"
#include "Level/Registry/TransformComponent.hpp"
#include "Level/Registry/MeshRendererComponent.hpp"
#include "Level/Registry/NameComponent.hpp"
#include "Level/Registry/SkyboxComponent.hpp"
#include "Level/Registry/CameraComponent.hpp"

#include "Renderer/RenderData.hpp"
#include "Renderer/Renderer.hpp" 
#include "Renderer/Camera.hpp"
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

#include "Renderer/RenderPipeline/RenderPipeline.hpp"
#include "Renderer/RenderPipeline/SkyboxRenderPipelineStage.hpp"
#include "Renderer/RenderPipeline/PBRRenderPipelineStage.hpp"
#include "Renderer/RenderPipeline/ColorspaceCorrectionRenderPipelinestage.hpp"
#include "Renderer/RenderPipeline/DebugViewRenderPipelineStage.hpp"
#include "Renderer/RenderPipeline/ClearRenderPipelineStage.hpp"
#include "Renderer/RenderPipeline/ShadowRenderPipelineStage.hpp"

#include "Serialization/ENTT/ComponentRegistry.hpp"


// Butterfly types.
#include "Renderer/D3D12Sampler.hpp"
#include "Renderer/D3D12Buffer.hpp"
#include "Renderer/D3D12Texture.hpp"

// Butterfly (Render)Graph
#include "Renderer/Graph/GraphBuilder.hpp"
#include "Renderer/Graph/Graph.hpp"
#include "Renderer/Graph/Blackboard.hpp"
#include "Renderer/Graph/GraphTransientResourceCache.hpp"

// Utils.
#include "Utils/Utils.hpp"
#include "Utils/Profiler.hpp"
