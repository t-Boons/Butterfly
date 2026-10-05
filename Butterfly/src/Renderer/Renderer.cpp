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

		m_windowResizeReceiver.Subscribe(Application::Get().GetWindow().Events().OnWindowResize, BF_BIND_FUNC_PARAM(&Renderer::OnWindowResize));
		m_windowRefreshReceiver.Subscribe(Application::Get().GetWindow().Events().OnWindowRefresh, BF_BIND_FUNC(&Renderer::OnWindowRefresh));

		m_resizeSize = { Application::Get().GetWindow().Width(), Application::Get().GetWindow().Height() };
		ApplyResize();

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
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
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
				*cpu = D3D12API()->DescriptorAllocatorSrvCbvUav()->CpuHandleFromHandle(handle);
				*gpu = D3D12API()->DescriptorAllocatorSrvCbvUav()->GpuHandleFromHandle(handle);
			};

		init_info.SrvDescriptorFreeFn =
			[](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu)
			{
				const uint32_t handle = D3D12API()->DescriptorAllocatorSrvCbvUav()->HandleFromGpuHandle(gpu);
				D3D12API()->DescriptorAllocatorSrvCbvUav()->FreeHandle(handle);
			};

		ImGui_ImplDX12_Init(&init_info);
	}

	const Viewport& Renderer::GetViewport(const ViewportHandle& handle) const
	{
		BF_CORE_ASSERT(handle.Valid(), "Renderer::GetViewport: ViewportHandle is invalid.");
		const auto it = GetFrameData().Viewports.find(handle);
		BF_CORE_ASSERT(it != GetFrameData().Viewports.end(), "Renderer::GetViewport: handle is not found in viewports.");
		return it->second;
	}

	Viewport& Renderer::GetViewport(const ViewportHandle& handle)
	{
		BF_CORE_ASSERT(handle.Valid(), "Renderer::GetViewport: ViewportHandle is invalid.");
		auto it = GetFrameData().Viewports.find(handle);
		BF_CORE_ASSERT(it != GetFrameData().Viewports.end(), "Renderer::GetViewport: handle is not found in viewports.");
		return it->second;
	}

	void Renderer::ImGUIImage(const ViewportHandle& handle)
	{
		BF_PROFILE_EVENT()

		ImVec2 size = ImGui::GetContentRegionAvail();
		if (size.x < 1.0f) size.x = 1.0f;
		if (size.y < 1.0f) size.y = 1.0f;

		BF_CORE_ASSERT(handle.Valid(), "Renderer::ImGUIImage: ViewportHandle is invalid.");

		if (GetFrameData().Viewports.empty())
		{
			BF_CORE_LOG_WARN("Renderer::ImGUIImage: No viewports found.");
			return;
		}

		auto it = GetFrameData().Viewports.find(handle);

		// When one of the viewports gets resized.
		if (!it->second.HasRenderTarget() || it->second.GetRenderTarget().Width() != size.x || it->second.GetRenderTarget().Height() != size.y)
		{
			WaitForInflightFrames();

			it->second.RenderTarget[it->second.FrameIndex].reset();
			it->second.GetGraphResources().Flush();

			{
				BFTextureDesc desc;
				desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				desc.Width = size.x;
				desc.Height = size.y;
				desc.ViewTypes = BFTextureDesc::ViewType::RenderTargettable | BFTextureDesc::ViewType::ShaderResource;
				desc.DebugName = "Viewport " + std::to_string(handle.m_index) + " RenderTarget";

				it->second.RenderTarget[it->second.FrameIndex] = MakeRef<BFTexture>(desc);
			}
			{
				BFTextureDesc desc;
				desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				desc.Width = size.x;
				desc.Height = size.y;
				desc.ViewTypes = BFTextureDesc::ViewType::DepthStencilable;
				desc.DebugName = "Viewport " + std::to_string(handle.m_index) + " DepthStencil";

				it->second.DepthStencil[it->second.FrameIndex] = MakeRef<BFTexture>(desc);
			}
			

			it->second.Events.OnResize.Broadcast(ViewportResizeEvent({ size.x, size.y }));
		}


		ImTextureID textureID = (ImTextureID)(uintptr_t)D3D12API()->DescriptorAllocatorSrvCbvUav()->GpuHandleFromHandle(it->second.GetRenderTarget().SRV().View()).ptr;
		ImGui::Image(textureID, { size.x, size.y });
	}

	void Renderer::Render()
	{
		BF_PROFILE_FRAME("Renderer::Render");

		if (m_resizePending)
		{
			ApplyResize();
		}

		m_frameData.FrameIndex = m_frameIndex;
		for (auto& it : m_frameData.Viewports)
		{
			Viewport& viewport = it.second;
			viewport.FrameIndex = m_frameIndex;
		}

		m_frameData.GetFence().Wait();
		m_frameData.GetCmdList().Reset();

		// Clear composite render target.
		{
			RasterPassStartInfo info;
			info.RenderTarget = &m_frameData.GetCompositeRenderTarget();
			info.ClearColor = { 0.1f, 0.02f, 0.02f, 1.0f };
			info.RenderTargetLoadOp = LoadOP::Clear;
			m_frameData.GetCmdList().StartRenderPass(info, "Composite clear pass");
			m_frameData.GetCmdList().EndRenderPass();
		}

		{
			BF_PROFILE_EVENT("Renderer::Render: ImGUI");

			// New ImGUI Frame.
			ImGui_ImplDX12_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			ImGuizmo::BeginFrame();

			m_ImGuiRenderEvent.Broadcast();
		}

		{
			BF_PROFILE_EVENT("Renderer::Render: Viewports");

			// Record all commands to render all viewports.
			for (auto& it : m_frameData.Viewports)
			{
				Viewport& viewport = it.second;

				GraphBuilder builder(viewport.GetGraphResources());

				m_frameData.GetCmdList().BeginGPUMarker("Viewport " + std::to_string(viewport.Handle.m_index));
				viewport.Events.OnPreRender.Broadcast(ViewportPrerenderEvent{ viewport });
				viewport.RenderPipeline->RecordPasses(ViewportRenderEvent{ builder, viewport });
				viewport.Events.OnRender.Broadcast(ViewportRenderEvent{ builder, viewport });
				viewport.Events.OnPostRender.Broadcast(ViewportPostRenderEvent{ builder, viewport });
				auto graph = builder.Create();
				graph->Execute(m_frameData.GetCmdList());
				delete graph;
				m_frameData.GetCmdList().EndGPUMarker();

				viewport.GetRenderTarget().Resource()->Transition(m_frameData.GetCmdList(), D3D12_RESOURCE_STATE_GENERIC_READ);
			}
		}

		{
			BF_PROFILE_EVENT("Renderer::Render: Render Submit");
			ImGui::Render();

			RasterPassStartInfo info;
			info.RenderTarget = &m_frameData.GetCompositeRenderTarget();
			info.RenderTargetLoadOp = LoadOP::Load;
			m_frameData.GetCmdList().StartRenderPass(info, "ImGUI Pass");
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_frameData.GetCmdList().List());

			Application::Get().GetWindow().Context().RecordCopyToBackBuffer(*m_frameData.GetCompositeRenderTarget().Resource(), m_frameData.GetCmdList());
			
			m_frameData.GetCmdList().EndRenderPass();


			m_frameData.GetCmdList().Close();
			D3D12API()->Queue(QueueType::Direct)->Execute(m_frameData.GetCmdList());
			m_frameData.GetFence().Signal(*D3D12API()->Queue(QueueType::Direct));
		}

		for (auto& it : m_frameData.Viewports)
		{
			Viewport& viewport = it.second;
			if (!viewport.ShouldRender)
			{
				continue;
			}

			viewport.RenderPipeline->PostRender();
		}

		Application::Get().GetWindow().Context().Present();

		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		m_previousFrame = m_frameIndex;
		m_frameIndex++;
		m_frameIndex = m_frameIndex % NUM_RENDER_BUFFERS;
	}

	ViewportHandle Renderer::AddViewport()
	{
		ViewportHandle handle;
		handle.m_index = m_viewportHandleIndex++;

		for (uint32_t i = 0; i < NUM_RENDER_BUFFERS; ++i)
		{
			Viewport viewport;
			viewport.GraphResources.fill(MakeRef<GraphTransientResourceCache>());
			viewport.Uniforms = MakeRef<BFUniformBuffer>(4096, "Frame " + std::to_string(i) + "Viewport " + std::to_string(handle.m_index) + " Uniforms");
			viewport.Handle = handle;

			BFStructuredBufferDesc desc;
			desc.Data = nullptr;
			desc.HeapType = BFHeapType::Upload;
			desc.NumElements = 128;
			desc.Stride = sizeof(ModelMatrixData);
			desc.DebugName = "Materials";

			viewport.ModelMatrices = MakeRef<BFStructuredBuffer>(desc);


			viewport.Materials = MakeRef<MaterialLibrary>();
			viewport.Lights = MakeRef<LightBuffer>();
			viewport.RenderPipeline = MakeRef<RenderPipeline>(*this);

			m_frameData.Viewports[handle] = viewport;
		}

		return handle;
	}


	void Renderer::RemoveViewport(const ViewportHandle& handle)
	{
		bool found = false;
		for (auto& it : m_frameData.Viewports)
		{
			if (it.first == handle)
			{
				m_frameData.Viewports.erase(it.first);
				found = true;
				break;
			}
		}

		if (!found)
		{
			BF_CORE_LOG_WARN("Renderer::RemoveViewport: ViewportHandle does not exist. Index: %u", handle.m_index);

		}
	}

	void Renderer::WaitForInflightFrames()
	{
		BF_PROFILE_EVENT()

		for (auto& fence : m_frameData.Fence)
		{
			fence->SignalAndWait(*D3D12API()->Queue(QueueType::Direct));
		}
	}

	void Renderer::OnWindowResize(const WindowResizeEvent& ev)
	{
		m_resizePending = true;
		m_resizeSize = { ev.Width, ev.Height };
	}

	void Renderer::OnWindowRefresh()
	{
		//Render();
	}

	void Renderer::ApplyResize()
	{
		m_resizePending = false;

		m_frameData.CmdList.fill(MakeRef<D3D12CommandList>());
		m_frameData.Fence.fill(MakeRef<D3D12Fence>());
		m_frameData.FrameIndex = 0;

		for (uint32_t j = 0; j < NUM_RENDER_BUFFERS; j++)
		{
			BFTextureDesc desc;
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.Width = static_cast<uint32_t>(m_resizeSize.x);
			desc.Height = static_cast<uint32_t>(m_resizeSize.y);
			desc.ViewTypes = BFTextureDesc::ViewType::RenderTargettable | BFTextureDesc::ViewType::ShaderResource;
			desc.DebugName = "Composite Render Target:" + std::to_string(j);
			m_frameData.CompositeRenderTarget[j] = MakeRef<BFTexture>(desc);
		}
	}

	Renderer::~Renderer()
	{
		WaitForInflightFrames();

		m_frameData.CompositeRenderTarget.fill(nullptr);
		m_frameData.CmdList.fill(nullptr);
		m_frameData.Fence.fill(nullptr);

		ImGui_ImplDX12_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
}