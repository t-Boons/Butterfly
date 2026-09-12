#include "Renderer/Renderer.hpp"
#include "Core/Window.hpp"

#include "Renderer/ModelLoading/ModelImporter.hpp"
#include "Core/Application.hpp"

#include "Renderer/GraphicsAPI.hpp"
#include "Renderer/D3D12/D3D12GraphicsAPI.hpp"
#include "Renderer/D3D12/D3D12DescriptorAllocator.hpp"
#include "Renderer/D3D12/D3D12CommandQueue.hpp"
#include "Renderer/D3D12/D3D12CommandList.hpp"
#include "Renderer/D3D12/D3D12GraphicsCommands.hpp"
#include "Renderer/D3D12/D3D12Shader.hpp"
#include "Renderer/D3D12/D3D12ShaderVariables.hpp"
#include "Renderer/D3D12Sampler.hpp"
#include "Renderer/D3D12/D3D12Pipeline.hpp"
#include "Renderer/Graph/GraphBuilder.hpp"
#include "Renderer/D3D12/D3D12View.hpp"
#include "Renderer/D3D12/D3D12Resource.hpp"

#include "Scene/Scene.hpp"
#include "Scene/Registry/MeshRendererComponent.hpp"
#include "Scene/Registry/TransformComponent.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_d3d12.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_internal.h"

#include "Asset/AssetManager.hpp"

#include "../../../ButterflyEditor/src/Tools/Camera.hpp"

namespace Butterfly
{
	Renderer::Renderer()
	{
		BF_PROFILE_EVENT()

		BFTextureDesc desc;
		desc.DebugName = "WhiteTexture";
		desc.Width = 1;
		desc.Height = 1;
		desc.Flags = BFTextureDesc::ShaderResource;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UINT;
		std::vector<uint8_t> data = { 225, 225, 225, 225 };
		m_whiteTexture = BFTexture::CreateTextureFromCPUBuffer(desc, data.data());

		Application::Get().GetWindow().Events().OnWindowResize.Subscribe(BF_BIND_FUNC_PARAM(&Renderer::OnWindowResize));
		Application::Get().GetWindow().Events().OnWindowRefresh.Subscribe(BF_BIND_FUNC(&Renderer::OnWindowRefresh));

		m_resizeSize = { Application::Get().GetWindow().Width(), Application::Get().GetWindow().Height() };
		InvalidateFrameDatas();


		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.IniFilename = "Editor/DefaultLayout.ini";

		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		ImGui_ImplGlfw_InitForOther(Application::Get().GetWindow().GLFWWindow(), true);

		ImGui_ImplDX12_InitInfo init_info;
		init_info.Device = D3D12API()->Device();
		init_info.NumFramesInFlight = NUM_RENDER_BUFFERS;
		init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		init_info.SrvDescriptorHeap = D3D12API()->DescriptorAllocatorSrvCbvUav()->Heap().Get();
		init_info.CommandQueue = D3D12API()->Queue(QueueType::Direct)->D3D12Queue();
		init_info.LegacySingleSrvCpuDescriptor = D3D12API()->DescriptorAllocatorSrvCbvUav()->Heap()->GetCPUDescriptorHandleForHeapStart();
		init_info.LegacySingleSrvGpuDescriptor = D3D12API()->DescriptorAllocatorSrvCbvUav()->Heap()->GetGPUDescriptorHandleForHeapStart();
		ImGui_ImplDX12_Init(&init_info);

		// We allocate a dummy because textureslot 1 is used by ImGUI for font rendering.
		D3D12API()->DescriptorAllocatorSrvCbvUav()->AllocateDummy();
	}

	void Renderer::ImGUIImage(const ViewportHandle& handle)
	{
		ImVec2 size = ImGui::GetContentRegionAvail();
		if (size.x < 1.0f) size.x = 1.0f;
		if (size.y < 1.0f) size.y = 1.0f;

		BF_CORE_ASSERT(handle.Valid(), "Renderer::ImGUIImage: ViewportHandle is invalid.");

		if (CurrentFrameData().Viewports.empty())
		{
			BF_CORE_LOG_WARN("Renderer::ImGUIImage: No viewports found.");
			return;
		}

		auto it = CurrentFrameData().Viewports.find(handle);

		// When one of the viewports gets resized.
		if (it == CurrentFrameData().Viewports.end() || !it->second.RenderTarget || it->second.RenderTarget->Width() != size.x || it->second.RenderTarget->Height() != size.y)
		{
			if (it != CurrentFrameData().Viewports.end() && it->second.RenderTarget)
			{
				WaitForInflightFrames();
				it->second.RenderTarget.reset();
			}

			it->second.GraphResources->Flush();

			BFTextureDesc desc;
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.Width = size.x;
			desc.Height = size.y;
			desc.Flags = BFTextureDesc::RenderTargettable | BFTextureDesc::ShaderResource;
			desc.DebugName = "Viewport " + std::to_string(handle.m_index) + " RenderTarget";

			it->second.RenderTarget = BFTexture::CreateTextureForGPU(desc);

			GetViewportEvents(handle).OnResize.Broadcast(ViewportResizeEvent({ size.x, size.y }));
		}


		ImTextureID textureID = (ImTextureID)(uintptr_t)D3D12API()->DescriptorAllocatorSrvCbvUav()->GpuHandleFromSrvHandle(it->second.RenderTarget->SRV().View()).ptr;
		ImGui::Image(textureID, { size.x, size.y });
	}

	void Renderer::Render()
	{
		BF_PROFILE_FRAME("Renderer::Render");

		if (m_resizePending)
		{
			ApplyResize();
		}

		FrameData& frame = m_frameDatas[m_frameIndex];
		frame.FrameIndex = m_frameIndex;
		frame.Fence->Wait();

		frame.CmdList->Reset();

		frame.CmdList->BeginGPUMarker("Render Frame -> " + std::to_string(frame.FrameIndex));

		// Clear composite render target.
		frame.CmdList->BeginGPUMarker("Composite Clear.");
		GraphicsCommands::ClearRenderTarget(*frame.CmdList, *frame.CompositeRenderTarget, { 0.05f, 0.05f, 0.05f, 1.0f });
		frame.CmdList->EndGPUMarker();


		// New ImGUI Frame.
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		m_ImGuiRenderEvent.Broadcast();


		// Record all commands to render all viewports.
		for (auto& it : frame.Viewports)
		{
			Viewport& viewport = it.second;

			if (!viewport.RenderTarget)
			{
				BF_CORE_LOG_WARN("Renderer::Render: Viewport %u has no render target. Skipping.", viewport.Handle.m_index);
				continue;
			}

			GraphBuilder builder(*viewport.GraphResources);

			frame.CmdList->BeginGPUMarker("Viewport " + std::to_string(viewport.Handle.m_index));
			GetViewportEvents(viewport.Handle).OnRender.Broadcast(ViewportRenderEvent{ builder, viewport });
			frame.CmdList->EndGPUMarker();

			auto graph = builder.Create();
			graph->Execute(*frame.CmdList);
			delete graph;

			viewport.RenderTarget->Resource()->Transition(*frame.CmdList, D3D12_RESOURCE_STATE_GENERIC_READ);
		}

		ImGui::Render();
		ID3D12DescriptorHeap* heaps[] = { D3D12API()->DescriptorAllocatorSrvCbvUav()->Heap().Get() };
		frame.CmdList->List()->SetDescriptorHeaps(_countof(heaps), heaps);
		GraphicsCommands::SetRenderTargets(*frame.CmdList, { frame.CompositeRenderTarget.get()}, nullptr);
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), frame.CmdList->List());

		Application::Get().GetWindow().Context().RecordCopyToBackBuffer(*frame.CompositeRenderTarget->Resource(), *frame.CmdList);


		frame.CmdList->EndGPUMarker();

		frame.CmdList->Close();
		D3D12API()->Queue(QueueType::Direct)->Execute(*frame.CmdList);
		frame.Fence->Signal(*D3D12API()->Queue(QueueType::Direct));

		Application::Get().GetWindow().Context().Present();

		m_previousFrame = m_frameIndex;
		m_frameIndex++;
		m_frameIndex = m_frameIndex % NUM_RENDER_BUFFERS;
	}

	ViewportEvents& Renderer::GetViewportEvents(const ViewportHandle& handle)
	{
		BF_CORE_ASSERT(handle.Valid(), "Renderer::GetViewportEvents: ViewportHandle is invalid.");
		auto it = m_viewportEvents.find(handle);
		BF_CORE_ASSERT(it != m_viewportEvents.end(), "Renderer::GetViewportEvents: ViewportHandle does not exist.");
		return it->second;
	}

	ViewportHandle Renderer::AddViewport()
	{
		ViewportHandle handle;
		handle.m_index = m_viewportHandleIndex++;
		m_existingViewportHandles.push_back(handle);

		InvalidateFrameDatas();

		m_viewportEvents[handle] = ViewportEvents();

		// Temp add the default render pipeline.
		GetViewportEvents(handle).OnRender.Subscribe(BF_BIND_FUNC_PARAM(&Renderer::RecordCmdList));


		return handle;
	}


	void Renderer::RemoveViewport(const ViewportHandle& handle)
	{
		auto it = std::find(m_existingViewportHandles.begin(), m_existingViewportHandles.end(), handle);

		if (it != m_existingViewportHandles.end())
		{
			m_existingViewportHandles.erase(it);
			InvalidateFrameDatas();
		}
		else
		{
			BF_CORE_LOG_WARN("Renderer::RemoveViewport: ViewportHandle does not exist. Index: %u", handle.m_index);
		}
	}

	struct UniformCameraData
	{
		glm::mat4 ViewProjection;
	};

	void Renderer::InvalidateFrameDatas()
	{
		WaitForInflightFrames();

		m_frameDatas.clear();
		m_frameDatas.resize(NUM_RENDER_BUFFERS);

		for (uint32_t i = 0; i < NUM_RENDER_BUFFERS; ++i)
		{
			BFTextureDesc desc;
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.Width = m_resizeSize.x;
			desc.Height = m_resizeSize.y;
			desc.Flags = BFTextureDesc::RenderTargettable | BFTextureDesc::ShaderResource;
			desc.DebugName = "Frame " + std::to_string(i) + " RenderTarget";

			FrameData& data = m_frameDatas[i];

			data.CompositeRenderTarget = BFTexture::CreateTextureForGPU(desc);

			data.CmdList = MakeRef<D3D12CommandList>();
			data.Fence = MakeRef<D3D12Fence>();

			data.Viewports.clear();


			// Recreate all viewports for this frame.
			for (uint32_t j = 0; j < m_existingViewportHandles.size(); ++j)
			{
				ViewportHandle& handle = m_existingViewportHandles[j];
				Viewport& viewport = data.Viewports[handle];

				viewport.Handle = handle;

				viewport.GraphResources = MakeRef<GraphTransientResourceCache>();
				viewport.Uniforms = MakeRef<BFUniformBuffer>(4096, "Frame " + std::to_string(i) + "Viewport " + std::to_string(handle.m_index) + " Uniforms");


				viewport.UniformCameraDataViewIndex = viewport.Uniforms->AllocView(sizeof(UniformCameraData));

				BFStructuredBufferDesc desc;
				desc.Data = nullptr;
				desc.HeapType = BFHeapType::Upload;
				desc.NumElements = 64;
				desc.Stride = sizeof(glm::mat4);
				desc.DebugName = "ModelMatrices";

				viewport.ModelMatrices = MakeRef<BFStructuredBuffer>(desc);
			}
		}
	}

	void Renderer::WaitForInflightFrames()
	{
		for (auto& frameData : m_frameDatas)
		{
			frameData.Fence->SignalAndWait(*D3D12API()->Queue(QueueType::Direct));
		}
	}

	void Renderer::RecordCmdList(const ViewportRenderEvent& ev)
	{
		GraphBuilder& builder = ev.Builder;
		Viewport& viewport = ev.Viewport;

		struct ForwardRenderer
		{
			BFRGTexture* DepthStencil;
			BFTexture* Comp;
		};

		ForwardRenderer* params = builder.AllocParameters<ForwardRenderer>();

		params->Comp = viewport.RenderTarget.get();


		BFTextureDesc desc2;
		desc2.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		desc2.Width = viewport.RenderTarget->Width();
		desc2.Height = viewport.RenderTarget->Height();
		desc2.Flags = BFTextureDesc::DepthStencilable;
		params->DepthStencil = builder.CreateTransientTexture("DepthStencil Viewport", desc2);

		builder.AddPass<ForwardRenderer>("Forward Model",
			[&](const ForwardRenderer& params, D3D12CommandList& list)
			{
				BF_PROFILE_EVENT_DYNAMIC("Forward Model pass");

				BFTexture& rt = *params.Comp;

				// Default Init stuff.
				list.List()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				GraphicsCommands::SetRenderTargets(list, { &rt }, params.DepthStencil->Resource().get());

				GraphicsCommands::ClearDepthStencil(list, *params.DepthStencil->Resource());
				GraphicsCommands::ClearRenderTarget(list, rt, { 0.05f, 0.1f, 0.15f, 1.0f });

				GraphicsCommands::SetFullscreenViewportAndRect(list, rt.Width(), rt.Height());

				BFPipelineBuilder psoBuilder;
				psoBuilder.PrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
				psoBuilder.RenderTargetFormats({ DXGI_FORMAT_R8G8B8A8_UNORM });
				psoBuilder.DepthStencilFormat({ DXGI_FORMAT_D24_UNORM_S8_UINT });
				psoBuilder.VertexShader(BFShaderCache::GetOrCreate(L"assets/Shaders/Forward_vert.hlsl", ShaderType::Vertex));
				psoBuilder.PixelShader(BFShaderCache::GetOrCreate(L"assets/Shaders/Forward_frag.hlsl", ShaderType::Pixel));
				psoBuilder.CullingMode(D3D12_CULL_MODE_FRONT);

				list.List()->SetPipelineState(psoBuilder.Create().GetHW());

				uint32_t entityIndex = 0;
				BFSampler sampler;
				auto view = Application::Get().GetScene().GetEntityRegistry().view<TransformComponent, MeshRendererComponent>();
				for (auto [entity, transform, meshRenderer] : view.each())
				{
					if(!meshRenderer.ContainsMesh())
					{
						continue;
					}

					UniformCameraData cameraData;
					cameraData.ViewProjection = Application::Get().GetBlackboard().Get<Camera>("ViewCamera")->ViewProjectionMatrix();

					viewport.Uniforms->Write(&cameraData, sizeof(UniformCameraData), viewport.UniformCameraDataViewIndex);

					const glm::mat4 model = transform.GetMatrix();
					viewport.ModelMatrices->Write(&model, sizeof(glm::mat4), entityIndex * sizeof(glm::mat4));

					AssetManager& as = Application::Get().GetAssetManager();
					MeshAsset* mesh = as.Resolve<MeshAsset>(meshRenderer.MeshHandle);
					ShaderVariables()
						.Add(mesh->GPUPositions->SRV().View())
						.Add(mesh->GPUNormals->SRV().View())
						.Add(mesh->GPUUVs->SRV().View())
						.Add(viewport.Uniforms->GetView(viewport.UniformCameraDataViewIndex)->View())
						.Add(sampler.View())
						.Add(m_whiteTexture->SRV().View())
						.Add(viewport.ModelMatrices->SRV().View())
						.Add(entityIndex)
						.Submit(list);

					list.List()->IASetIndexBuffer(&mesh->GPUIndices->IBV());
					list.List()->DrawIndexedInstanced(mesh->GPUIndices->NumElements(), 1, 0, 0, 0);

					entityIndex++;
				}
			});
	}

	void Renderer::OnWindowResize(const WindowResizeEvent& ev)
	{
		m_resizePending = true;
		m_resizeSize = { ev.Width, ev.Height };
	}

	void Renderer::OnWindowRefresh()
	{
		Render();
	}

	void Renderer::ApplyResize()
	{
		m_resizePending = false;

		InvalidateFrameDatas();
	}

	Renderer::~Renderer()
	{
		WaitForInflightFrames();

		m_frameDatas.clear();

		ImGui_ImplDX12_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
}