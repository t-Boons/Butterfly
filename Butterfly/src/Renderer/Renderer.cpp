#include "Renderer/Renderer.hpp"
#include "Core/Application.hpp"

#include "imgui/imgui_impl_dx12.h"
#include "imgui/imgui_impl_glfw.h"

#include "Renderer/RenderPipeline/RenderPipeline.hpp"

namespace Butterfly
{
	Renderer::Renderer()
	{
		BF_PROFILE_EVENT()

		m_renderPipeline = MakeRef<RenderPipeline>(*this);

		m_windowResizeReceiver.Subscribe(Application::Get().GetWindow().Events().OnWindowResize, BF_BIND_FUNC_PARAM(&Renderer::OnWindowResize));
		m_windowRefreshReceiver.Subscribe(Application::Get().GetWindow().Events().OnWindowRefresh, BF_BIND_FUNC(&Renderer::OnWindowRefresh));

		m_resizeSize = { Application::Get().GetWindow().Width(), Application::Get().GetWindow().Height() };
		InvalidateFrameDatas();


		ImGui::CreateContext();

		ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());

		ImGuiIO& io = ImGui::GetIO();
		io.IniFilename = "Editor/DefaultLayout.ini";

		{
			ImFontConfig config;
			config.PixelSnapH = true;

			const std::string filepath = "Assets/Fonts/Roboto-Regular.ttf";

			BF_CORE_ASSERT(
				std::filesystem::exists(filepath),
				"Renderer::Renderer: Font file does not exist: %s",
				filepath.c_str()
			);

			io.Fonts->AddFontFromFileTTF(filepath.c_str(), 14.0f, &config);
		}

		{
			ImFontConfig config;
			config.PixelSnapH = true;

			static const ImWchar iconRanges[] = { 0xf000, 0xf8ff, 0 };

			const std::string filepath = "Assets/Fonts/Font_Awesome_7_Free-Solid-900.otf";
			BF_CORE_ASSERT(std::filesystem::exists(filepath), "Renderer::Renderer: Font file does not exist: %s", filepath.c_str());

			io.Fonts->AddFontFromFileTTF(filepath.c_str(), 9.0f, &config, iconRanges);
			io.Fonts->AddFontFromFileTTF(filepath.c_str(), 128.0f, &config, iconRanges);
		}

		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		ImGui_ImplGlfw_InitForOther(Application::Get().GetWindow().GLFWWindow(), true);

		ImGui_ImplDX12_InitInfo init_info{};
		init_info.Device = D3D12API()->Device();
		init_info.NumFramesInFlight = NUM_RENDER_BUFFERS;
		init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		init_info.SrvDescriptorHeap = D3D12API()->DescriptorAllocatorSrvCbvUav()->Heap().Get();
		init_info.CommandQueue = D3D12API()->Queue(QueueType::Direct)->D3D12Queue();

		init_info.SrvDescriptorAllocFn =
			[](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* cpu, D3D12_GPU_DESCRIPTOR_HANDLE* gpu)
			{
				const uint32_t handle = D3D12API()->DescriptorAllocatorSrvCbvUav()->Allocate();
				*cpu = D3D12API()->DescriptorAllocatorSrvCbvUav()->CpuHandleFromSrvHandle(handle);
				*gpu = D3D12API()->DescriptorAllocatorSrvCbvUav()->GpuHandleFromSrvHandle(handle);
			};

		init_info.SrvDescriptorFreeFn =
			[](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu)
			{
				const uint32_t handle = D3D12API()->DescriptorAllocatorSrvCbvUav()->HandleFromGpuHandle(gpu);
				D3D12API()->DescriptorAllocatorSrvCbvUav()->FreeHandle(handle);
			};

		ImGui_ImplDX12_Init(&init_info);
	}

	const Viewport& Renderer::GetViewport(const ViewportHandle& handle)
	{
		BF_CORE_ASSERT(handle.Valid(), "Renderer::GetViewport: ViewportHandle is invalid.");
		auto it = GetCurrentFrameData().Viewports.find(handle);
		BF_CORE_ASSERT(it != GetCurrentFrameData().Viewports.end(), "Renderer::GetViewport: handle is not found in viewports.");
		return it->second;
	}

	void Renderer::ImGUIImage(const ViewportHandle& handle)
	{
		ImVec2 size = ImGui::GetContentRegionAvail();
		if (size.x < 1.0f) size.x = 1.0f;
		if (size.y < 1.0f) size.y = 1.0f;

		BF_CORE_ASSERT(handle.Valid(), "Renderer::ImGUIImage: ViewportHandle is invalid.");

		if (GetCurrentFrameData().Viewports.empty())
		{
			BF_CORE_LOG_WARN("Renderer::ImGUIImage: No viewports found.");
			return;
		}

		auto it = GetCurrentFrameData().Viewports.find(handle);

		// When one of the viewports gets resized.
		if (it == GetCurrentFrameData().Viewports.end() || !it->second.RenderTarget || it->second.RenderTarget->Width() != size.x || it->second.RenderTarget->Height() != size.y)
		{
			if (it != GetCurrentFrameData().Viewports.end() && it->second.RenderTarget)
			{
				WaitForInflightFrames();
				it->second.RenderTarget.reset();
			}

			it->second.GraphResources->Flush();

			{
				BFTextureDesc desc;
				desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				desc.Width = size.x;
				desc.Height = size.y;
				desc.Flags = BFTextureDesc::RenderTargettable | BFTextureDesc::ShaderResource;
				desc.DebugName = "Viewport " + std::to_string(handle.m_index) + " RenderTarget";

				it->second.RenderTarget = BFTexture::CreateTextureForGPU(desc);
			}
			{
				BFTextureDesc desc;
				desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				desc.Width = size.x;
				desc.Height = size.y;
				desc.Flags = BFTextureDesc::DepthStencilable;
				desc.DebugName = "Viewport " + std::to_string(handle.m_index) + " DepthStencil";

				it->second.DepthStencil = BFTexture::CreateTextureForGPU(desc);
			}
			

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

		frame.CmdList->BeginGPUMarker("Render");

		// Clear composite render target.
		GraphicsCommands::ClearRenderTarget(*frame.CmdList, *frame.CompositeRenderTarget, { 0.05f, 0.05f, 0.05f, 1.0f });

		// New ImGUI Frame.
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();

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
			GetViewportEvents(viewport.Handle).OnPreRender.Broadcast(ViewportPrerenderEvent{ viewport });
			m_renderPipeline->RecordPasses(ViewportRenderEvent{ builder, viewport });
			GetViewportEvents(viewport.Handle).OnRender.Broadcast(ViewportRenderEvent{ builder, viewport });
			GetViewportEvents(viewport.Handle).OnPostRender.Broadcast(ViewportPostRenderEvent{ builder, viewport });
			auto graph = builder.Create();
			graph->Execute(*frame.CmdList);
			delete graph;

			viewport.RenderTarget->Resource()->Transition(*frame.CmdList, D3D12_RESOURCE_STATE_GENERIC_READ);
			frame.CmdList->EndGPUMarker();
		}

		{
			frame.CmdList->BeginGPUMarker("ImGUI");
			ImGui::Render();
			GraphicsCommands::SetRenderTargets(*frame.CmdList, { frame.CompositeRenderTarget.get() }, nullptr);
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), frame.CmdList->List());

			Application::Get().GetWindow().Context().RecordCopyToBackBuffer(*frame.CompositeRenderTarget->Resource(), *frame.CmdList);
			frame.CmdList->EndGPUMarker();
		}

		frame.CmdList->EndGPUMarker();
		frame.CmdList->Close();
		D3D12API()->Queue(QueueType::Direct)->Execute(*frame.CmdList);
		frame.Fence->Signal(*D3D12API()->Queue(QueueType::Direct));

		m_renderPipeline->PostRender();

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

				{
					BFStructuredBufferDesc desc;
					desc.Data = nullptr;
					desc.HeapType = BFHeapType::Upload;
					desc.NumElements = 128;
					desc.Stride = sizeof(ModelMatrixData);
					desc.DebugName = "Materials";

					viewport.ModelMatrices = MakeRef<BFStructuredBuffer>(desc);
				}

				viewport.Materials = MakeRef<MaterialLibrary>();
				viewport.Lights = MakeRef<LightBuffer>();
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