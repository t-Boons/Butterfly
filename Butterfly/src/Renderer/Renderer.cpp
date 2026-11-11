#include "Renderer/Renderer.hpp"

#include "imgui/imgui_impl_dx12.h"
#include "imgui/imgui_impl_glfw.h"

#include "Core/Application.hpp"
#include "Renderer/D3D12Texture.hpp"
#include "Renderer/RenderData.hpp"
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
		init_info.RTVFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
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

	void Renderer::ImGUIImage(const RefPtr<Viewport>& viewport)
	{
		BF_PROFILE_EVENT()

		ImVec2 size = ImGui::GetContentRegionAvail();
		if (size.x < 1.0f) size.x = 1.0f;
		if (size.y < 1.0f) size.y = 1.0f;

		if (GetFrameData().Viewports.empty())
		{
			BF_CORE_LOG_WARN("Renderer::ImGUIImage: No viewports found.");
			return;
		}

		// When one of the viewports gets resized.
		if (!viewport->HasRenderTarget() || viewport->GetRenderTarget().Width() != size.x || viewport->GetRenderTarget().Height() != size.y)
		{
			FlushRenderCommands();

			viewport->m_renderTarget[viewport->m_frameIndex].reset();
			viewport->m_depthStencil[viewport->m_frameIndex].reset();
			viewport->GetGraphResources().Flush();

			{
				BFTextureDesc desc;
				desc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
				desc.Width = size.x;
				desc.Height = size.y;
				desc.ViewTypes = BFTextureDesc::ViewType::RenderTargettable | BFTextureDesc::ViewType::ShaderResource;
				desc.DebugName = "Viewport " + viewport->GetName() + " RenderTarget";

				viewport->m_renderTarget[viewport->m_frameIndex] = MakeRef<BFTexture>(desc);
			}
			{
				BFTextureDesc desc;
				desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				desc.Width = size.x;
				desc.Height = size.y;
				desc.ViewTypes = BFTextureDesc::ViewType::DepthStencilable;
				desc.DebugName = "Viewport " + viewport->GetName() + " DepthStencil";

				viewport->m_depthStencil[viewport->m_frameIndex] = MakeRef<BFTexture>(desc);
			}
			

			viewport->GetEvents().OnResize.Broadcast(ViewportResizeEvent({size.x, size.y}));
		}


		ImTextureID textureID = (ImTextureID)(uintptr_t)D3D12API()->DescriptorAllocatorSrvCbvUav()->GpuHandleFromHandle(viewport->GetRenderTarget().SRV().View()).ptr;
		ImGui::Image(textureID, { size.x, size.y });
	}

	void Renderer::Render()
	{
		BF_PROFILE_FRAME("Renderer::Render");

		m_frameData.FrameIndex = m_frameIndex;
		for (auto& it : m_frameData.Viewports)
		{
			Viewport& viewport = *it;
			viewport.m_frameIndex = m_frameIndex;
		}

		m_frameData.GetFence().Wait();
		m_frameData.GetCmdList().Reset();

		if (m_resizePending)
		{
			ApplyResize();
		}

		// Remove viewports that are no longer referenced by any other part of the application.
		auto isDeadFunc = [](const RefPtr<Viewport>& vp) { return vp.use_count() == 1; };
		if (std::any_of(m_frameData.Viewports.begin(), m_frameData.Viewports.end(), isDeadFunc))
		{
			FlushRenderCommands();
			std::erase_if(m_frameData.Viewports, isDeadFunc);
		}



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
			BF_PROFILE_EVENT("Renderer::Render Viewports PreRender");

			// Pre-render all viewports.
			for (auto& it : m_frameData.Viewports)
			{
				Viewport& viewport = *it;
				viewport.GetRenderPipeline().PreRender(ViewportPrerenderEvent{ viewport });
				viewport.GetEvents().OnPreRender.Broadcast(ViewportPrerenderEvent{ viewport });
			}
		}

		{
			BF_PROFILE_EVENT("Renderer::Render Viewports Render");

			// Record all commands to render all viewports.
			for (auto& it : m_frameData.Viewports)
			{
				Viewport& viewport = *it;

				GraphBuilder builder(viewport.GetGraphResources());

				m_frameData.GetCmdList().BeginGPUMarker("Viewport " + viewport.GetName());

				viewport.GetRenderPipeline().RecordPasses(ViewportRenderEvent{ builder, viewport });
				viewport.GetEvents().OnRender.Broadcast(ViewportRenderEvent{ builder, viewport });

				auto graph = builder.Create();
				graph->Execute(m_frameData.GetCmdList());
				delete graph;
				m_frameData.GetCmdList().EndGPUMarker();

				if (viewport.HasRenderTarget())
				{
					viewport.GetRenderTarget().Resource()->Transition(m_frameData.GetCmdList(), D3D12_RESOURCE_STATE_GENERIC_READ);
				}
			}
		}

		{
			// Submit all ImGUI and viewport render commands to the GPU.
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

		{
			BF_PROFILE_EVENT("Renderer::Render Viewports PostRender");

			// Post render all viewports.
			for (auto& it : m_frameData.Viewports)
			{
				Viewport& viewport = *it;
				viewport.GetRenderPipeline().PostRender();
				viewport.GetEvents().OnPostRender.Broadcast();
			}
		}
		
		// Present the final image to the screen.
		Application::Get().GetWindow().Context().Present();

		// Update ImGUI viewports outside of the screen.
		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		// Update the frame index for the next frame.
		m_previousFrame = m_frameIndex;
		m_frameIndex++;
		m_frameIndex = m_frameIndex % NUM_RENDER_BUFFERS;
	}

	RefPtr<Viewport> Renderer::CreateViewport(const std::string& name)
	{
		RefPtr<Viewport> viewport = MakeRef<Viewport>();
		m_frameData.Viewports.insert(viewport);

		viewport->m_viewportName = name;

		for (uint32_t i = 0; i < NUM_RENDER_BUFFERS; ++i)
		{
			{
				BFTextureDesc desc;
				desc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
				desc.Width = 1;
				desc.Height = 1;
				desc.ViewTypes = BFTextureDesc::ViewType::RenderTargettable | BFTextureDesc::ViewType::ShaderResource;
				desc.DebugName = "Viewport " + viewport->GetName() + " RenderTarget";

				viewport->m_renderTarget[i] = MakeRef<BFTexture>(desc);
			}
			{
				BFTextureDesc desc;
				desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				desc.Width = 1;
				desc.Height = 1;
				desc.ViewTypes = BFTextureDesc::ViewType::DepthStencilable;
				desc.DebugName = "Viewport " + viewport->GetName() + " DepthStencil";

				viewport->m_depthStencil[i] = MakeRef<BFTexture>(desc);
			}

			viewport->m_graphResources[i] = MakeRef<GraphTransientResourceCache>();
		}

		viewport->m_renderData = MakeRef<BFRenderData>();
		viewport->m_renderPipeline = MakeRef<RenderPipeline>(*this);

		return viewport;
	}

	void Renderer::FlushRenderCommands()
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
			desc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
			desc.Width = static_cast<uint32_t>(m_resizeSize.x);
			desc.Height = static_cast<uint32_t>(m_resizeSize.y);
			desc.ViewTypes = BFTextureDesc::ViewType::RenderTargettable | BFTextureDesc::ViewType::ShaderResource;
			desc.DebugName = "Composite Render Target:" + std::to_string(j);
			m_frameData.CompositeRenderTarget[j] = MakeRef<BFTexture>(desc);
		}
	}

	Renderer::~Renderer()
	{
		FlushRenderCommands();

		m_frameData.CompositeRenderTarget.fill(nullptr);
		m_frameData.CmdList.fill(nullptr);
		m_frameData.Fence.fill(nullptr);

		ImGui_ImplDX12_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
}