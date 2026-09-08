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
#include "Scene/Registry/MeshRenderer.hpp"
#include "Scene/Registry/Transform.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_d3d12.h"
#include "imgui/imgui_impl_glfw.h"

#include "../../../ButterflyDemo/src/Tools/Camera.hpp"

namespace Butterfly
{
	void Renderer::Init()
	{
		BF_PROFILE_EVENT()

		// Create resouce cache.
		m_blackBoard = ScopePtr<Blackboard>(new Blackboard());

		D3D12API()->DescriptorAllocatorSrvCbvUav()->AllocateDummy(); // Because ImGUI takes slot 0;

		Application::Get().GetWindow().Events().OnWindowResize.Subscribe([=](const WindowResizeEvent& ev)
			{
				m_resizePending = true;
				m_resizeSize = { ev.Width, ev.Height };
			});

		Application::Get().GetWindow().Events().OnWindowRefresh.Subscribe([=](const WindowRefreshEvent&)
			{
				Render();
			});

		FrameCreateData createData;
		createData.Size = { Application::Get().GetWindow().Width(), Application::Get().GetWindow().Height() };
		InvalidateFrameDatas(createData);


		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
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

		OnPreFrameRecorded.Broadcast(frame);


		frame.CmdList->BeginGPUMarker("Render Frame -> " + std::to_string(frame.FrameIndex));

		frame.CmdList->BeginGPUMarker("Composite Clear.");
		GraphicsCommands::ClearRenderTarget(*frame.CmdList, *frame.RenderTarget, { 0.0, 0.05f, 0.1f, 1.0f });
		frame.CmdList->EndGPUMarker();


		ImGui_ImplDX12_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::DockSpaceOverViewport();

		for (uint32_t i = 0; i < m_numViewports; i++)
		{
			frame.CmdList->BeginGPUMarker("Viewport " + std::to_string(i));
			const std::string name = "Viewport " + std::to_string(i);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			ImGui::Begin(name.c_str(), nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground);

			ImVec2 size = ImGui::GetContentRegionAvail();
			if (size.x < 1.0f) size.x = 1.0f;
			if (size.y < 1.0f) size.y = 1.0f;

			// When one of the viewports gets resized.
			if (!frame.Viewports[i].RenderTarget || frame.Viewports[i].RenderTarget->Width() != size.x || frame.Viewports[i].RenderTarget->Height() != size.y)
			{
				if (frame.Viewports[i].RenderTarget)
				{
					WaitForInflightFrames();
					frame.Viewports[i].RenderTarget.reset();
				}

				frame.Viewports[i].GraphResources->Flush();

				BFTextureDesc desc;
				desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				desc.Width = size.x;
				desc.Height = size.y;
				desc.Flags = BFTextureDesc::RenderTargettable | BFTextureDesc::ShaderResource;
				desc.DebugName = "Viewport " + std::to_string(i) + " RenderTarget";

				frame.Viewports[i].RenderTarget = BFTexture::CreateTextureForGPU(desc);

				OnViewportResize.Broadcast(ViewportResizeEvent({size.x, size.y }));
			}

			ImTextureID textureID = (ImTextureID)(uintptr_t)D3D12API()->DescriptorAllocatorSrvCbvUav()->GpuHandleFromSrvHandle(frame.Viewports[i].RenderTarget->SRV().View()).ptr;

			frame.Viewports[i].RenderTarget->Resource()->Transition(*frame.CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
			RecordCmdList(frame, i);
			frame.Viewports[i].RenderTarget->Resource()->Transition(*frame.CmdList, D3D12_RESOURCE_STATE_GENERIC_READ);
			ImGui::Image(textureID, size);

			ImGui::PopStyleVar(2);
			ImGui::End();

			OnFrameRecorded.Broadcast(frame);

			frame.CmdList->EndGPUMarker();
		}

		ImGui::Render();
		ID3D12DescriptorHeap* heaps[] = { D3D12API()->DescriptorAllocatorSrvCbvUav()->Heap().Get() };
		frame.CmdList->List()->SetDescriptorHeaps(_countof(heaps), heaps);
		GraphicsCommands::SetRenderTargets(*frame.CmdList, { frame.RenderTarget.get()}, nullptr);
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), frame.CmdList->List());

		Application::Get().GetWindow().Context().RecordCopyToBackBuffer(*frame.RenderTarget->Resource(), *frame.CmdList);


		frame.CmdList->EndGPUMarker();

		frame.CmdList->Close();
		D3D12API()->Queue(QueueType::Direct)->Execute(*frame.CmdList);
		frame.Fence->Signal(*D3D12API()->Queue(QueueType::Direct));

		Application::Get().GetWindow().Context().Present();

		m_previousFrame = m_frameIndex;
		m_frameIndex++;
		m_frameIndex = m_frameIndex % NUM_RENDER_BUFFERS;
	}

	struct UniformCameraData
	{
		glm::mat4 ViewProjection;
		glm::mat4 Model;
	};

	void Renderer::InvalidateFrameDatas(const FrameCreateData& createData)
	{
		WaitForInflightFrames();

		m_frameDatas.clear();
		m_frameDatas.resize(NUM_RENDER_BUFFERS);

		for (uint32_t i = 0; i < NUM_RENDER_BUFFERS; ++i)
		{
			BFTextureDesc desc;
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.Width = createData.Size.x;
			desc.Height = createData.Size.y;
			desc.Flags = BFTextureDesc::RenderTargettable | BFTextureDesc::ShaderResource;
			desc.DebugName = "Frame " + std::to_string(i) + " RenderTarget";

			m_frameDatas[i].RenderTarget = BFTexture::CreateTextureForGPU(desc);

			m_frameDatas[i].CmdList = MakeRef<D3D12CommandList>();
			m_frameDatas[i].Fence = MakeRef<D3D12Fence>();
			m_frameDatas[i].FramePresentable = false;

			m_frameDatas[i].Viewports.clear();
			m_frameDatas[i].Viewports.resize(m_numViewports);


			for (uint32_t j = 0; j < m_frameDatas[i].Viewports.size(); j++)
			{
				Viewport& viewport = m_frameDatas[i].Viewports[j];
				viewport.GraphResources = MakeRef<GraphTransientResourceCache>();
				viewport.Uniforms = MakeRef<BFUniformBuffer>(4096, "Frame " + std::to_string(i) + "Viewport " + std::to_string(j) + " Uniforms");


				m_frameDatas[i].UniformCameraDataViewIndex = viewport.Uniforms->AllocView(sizeof(UniformCameraData));
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

	void Renderer::RecordCmdList(FrameData& frameData, uint32_t viewportIndex)
	{
		Viewport& viewport = frameData.Viewports[viewportIndex];
		GraphBuilder builder(*frameData.Viewports[viewportIndex].GraphResources);

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
		params->DepthStencil = builder.CreateTransientTexture("DepthStencil Viewport " + std::to_string(viewportIndex), desc2);

		builder.AddPass<ForwardRenderer>("Forward Model",
			[&](const ForwardRenderer& params, D3D12CommandList& list)
			{
				BF_PROFILE_EVENT_DYNAMIC("Forward Model pass");

				BFTexture& rt = *params.Comp;

				// Default Init stuff.
				list.List()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				GraphicsCommands::SetRenderTargets(list, { &rt }, params.DepthStencil->Resource().get());

				GraphicsCommands::ClearDepthStencil(list, *params.DepthStencil->Resource());
				GraphicsCommands::ClearRenderTarget(list, rt, { 0.05f, 0.1f, 0.25f, 1.0f });

				GraphicsCommands::SetFullscreenViewportAndRect(list, rt.Width(), rt.Height());

				BFPipelineBuilder psoBuilder;
				psoBuilder.PrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
				psoBuilder.RenderTargetFormats({ DXGI_FORMAT_R8G8B8A8_UNORM });
				psoBuilder.DepthStencilFormat({ DXGI_FORMAT_D24_UNORM_S8_UINT });
				psoBuilder.VertexShader(BFShaderCache::GetOrCreate(L"assets/Shaders/Forward_vert.hlsl", ShaderType::Vertex));
				psoBuilder.PixelShader(BFShaderCache::GetOrCreate(L"assets/Shaders/Forward_frag.hlsl", ShaderType::Pixel));
				psoBuilder.CullingMode(D3D12_CULL_MODE_FRONT);

				list.List()->SetPipelineState(psoBuilder.Create().GetHW());

				BFSampler sampler;
				auto view = Application::Get().GetScene().GetEntityRegistry().view<Transform, MeshRenderer>();
				for (auto [entity, transform, meshRenderer] : view.each())
				{
					if (!meshRenderer.m_meshLoaded)
					{
						continue;
					}

					UniformCameraData cameraData;
					cameraData.Model = transform.GetMatrix();
					cameraData.ViewProjection = Application::Get().GetBlackboard().Get<Camera>("ViewCamera")->ViewProjectionMatrix();

					frameData.Viewports[viewportIndex].Uniforms->Write(&cameraData, sizeof(UniformCameraData), frameData.UniformCameraDataViewIndex);

					ShaderVariables()
						.Add(meshRenderer.m_modelPositions->SRV().View())
						.Add(meshRenderer.m_modelNormals->SRV().View())
						.Add(meshRenderer.m_modelUVS->SRV().View())
						.Add(frameData.Viewports[viewportIndex].Uniforms	->GetView(frameData.UniformCameraDataViewIndex)->View())
						.Add(sampler.View())
						.Add(meshRenderer.m_modelAlbedo->SRV().View())
						.Submit(list);

					list.List()->IASetIndexBuffer(&meshRenderer.m_modelIndices->IBV());
					list.List()->DrawIndexedInstanced(meshRenderer.m_modelIndices->NumElements(), 1, 0, 0, 0);
				}
			});

		auto graph = builder.Create();
		graph->Execute(*frameData.CmdList);
		delete graph;
	}

	void Renderer::ApplyResize()
	{
		m_resizePending = false;

		FrameCreateData createData;
		createData.Size = m_resizeSize;

		InvalidateFrameDatas(createData);
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