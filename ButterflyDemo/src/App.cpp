#include "App.hpp"
#include "GraphExamples/DeferredSponza.hpp"
#include "GraphExamples/ForwardSponza.hpp"
#include <numeric>

namespace Butterfly
{
	void TraversePassDeps(const Graph* graph, const PassBase* parent)
	{
		BF_PROFILE_EVENT()
		auto deps = graph->PassDependencies(parent);
		if (!deps.empty())
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 1.0f, 1.0f)); // Red text
			if (ImGui::TreeNode(parent->Name().c_str()))
			{
				for (size_t j = 0; j < deps.size(); j++)
					TraversePassDeps(graph, deps[j]);
				ImGui::TreePop();
			}
			ImGui::PopStyleColor();
		}
		else
			ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), parent->Name().c_str());
	}

	void App::Init()
	{
		glm::vec3 normal = glm::vec3{ 1.0f, 0.0001f, 1.0f } + 1.0f;
		glm::vec3 normal2 = glm::vec3{ 1.0f, 0.0001f, -1.0f } + 1.0f;

		float z1 = glm::sqrt(2.0 - (normal.x * normal.x) - (normal.y * normal.y));
		glm::vec3 normal0 = glm::normalize(glm::vec3(normal.x, normal.y, z1));

		float z2 = glm::sqrt(2.0 - (normal.x * normal.x) - (normal.y * normal.y));
		glm::vec3 normal1 = glm::normalize(glm::vec3(normal2.x, normal2.y, z2));



		BF_PROFILE_EVENT()

		bool useDebug = true;
#ifdef NDEBUG
		useDebug = false;
#endif
		// Initialize
		{
			GraphicsAPI::Init(GraphicsAPIType::D3D12, useDebug);

			m_window = ScopePtr<Butterfly::Window>(new Butterfly::Window("Butterfly Renderer", 1280, 720));

			// Bind window events.
			m_window->Events().OnWindowResize.Subscribe(BF_BIND_FUNC(&App::OnResize));
			m_imGUi.Init(m_window.get());

			StaticWindow = m_window.get();
			m_input.Init(m_window.get());
		}

		// Create resouce cache.
		m_graphResources = ScopePtr<GraphTransientResourceCache>(new GraphTransientResourceCache());
		m_blackBoard = ScopePtr<Blackboard>(new Blackboard());

		DX12API()->DescriptorAllocatorSrvCbvUav()->AllocateDummy(); // Because ImGUI takes slot 0;


		SetCompositeBufferResolutionIfChanged(m_window->Width(), m_window->Height());

		WindowResizeEvent ev;
		ev.Width = m_window->Width();
		ev.Height = m_window->Height();
		OnResize(ev);

		// Create command list.
		auto genericCmdList = Graphics->CreateCommandList(QueueType::Direct);
		m_cmdList = std::dynamic_pointer_cast<DX12CommandList>(genericCmdList);


		m_currentDemo = m_demos[0];
		m_spectatorCam.GetCamera()->SetPosition({ 0.0f, 1.0f, 0.0f });
	}

	void App::Tick()
	{
		BF_PROFILE_FRAME("MainThread");
		m_spectatorCam.Tick(m_input, m_window->DeltaTime());
		ShouldShutDown = m_window->ShouldClose();

		if (m_input.IsKeyDown(BFB_F))
		{
			m_imguiEnabled = !m_imguiEnabled;
		}
	}

	void App::Render()
	{
		m_cmdList->BeginGPUMarker("Initial screen clear.");

		GraphicsCommands::ClearRenderTarget(*m_cmdList, *CompositeTexture, { 0.0, 0.05f, 0.1f, 1.0f });
		m_cmdList->EndGPUMarker();
		m_cmdList->Close();
		DX12API()->Queue(QueueType::Direct)->Execute(*m_cmdList);
		DX12API()->Queue(QueueType::Direct)->WaitForFence();
		m_cmdList->Reset();

		GraphBuilder builder(*m_graphResources);

		if (m_graphExample)
		{
			m_graphExample->Record(builder, *m_spectatorCam.GetCamera().get());
		}


		auto graph = ScopePtr<const Graph>(builder.Create());

		graph->Execute();

		if (m_imguiEnabled)
		{
			ImGuiRender(graph.get());
		}


		{
			BF_PROFILE_EVENT("SwapBuffers")
		    if (m_imguiEnabled)
		    {
		    	m_window->Context().DisplayTexture(*m_blackBoard->Get<BFTexture>("Screen").Resource());
		    }
			else
			{
				SetCompositeBufferResolutionIfChanged(m_window->Width(), m_window->Height());
				m_window->Context().DisplayTexture(*CompositeTexture->Resource());
			}

			m_input.Poll();
			m_window->Update();
		}
	}

	void App::ShutDown()
	{
		BF_PROFILE_EVENT()

		FullscreenQuad::ShutDown();
	}

	void App::ImGuiRender(const Graph* graph)
	{
		BF_PROFILE_EVENT()

		m_cmdList->BeginGPUMarker("ImGui");

		// IMGUI DEBUG DATA.
		m_imGUi.BeginFrame();

		ImGui::DockSpaceOverViewport();
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::Begin("Texture Window", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground);
		ImVec2 newSize = ImGui::GetContentRegionAvail();
		SetCompositeBufferResolutionIfChanged((uint32_t)newSize.x, (uint32_t)newSize.y);
		BFTexture& tex = *CompositeTexture;
		ImTextureID textureID = (ImTextureID)(uintptr_t)DX12API()->DescriptorAllocatorSrvCbvUav()->GpuHandleFromSrvHandle(tex.SRV().View()).ptr;

		ImGui::Image(textureID, newSize);
		
		ImGui::End();
		ImGui::PopStyleVar();




		ImGui::Begin("RenderGraph.");

		// Select demo.
		if (ImGui::BeginCombo("Select Demo", m_currentDemo.data()))
		{
			for (int n = 0; n < IM_ARRAYSIZE(m_demos); n++)
			{
				const bool is_selected = (m_currentDemo == m_demos[n]);

				if (ImGui::Selectable(m_demos[n].data(), is_selected))
				{
					m_currentDemo = m_demos[n].data();
					if (is_selected) ImGui::SetItemDefaultFocus();

					m_graphExample.reset();

					if (m_currentDemo == "DeferredSponza")
					{
						m_graphExample = ScopePtr<GraphExamples::DeferredSponza>(new GraphExamples::DeferredSponza());
						m_graphExample->Init();
					}

					if (m_currentDemo == "ForwardSponza")
					{
						m_graphExample = ScopePtr<GraphExamples::ForwardSponza>(new GraphExamples::ForwardSponza());
					}

					if (m_graphExample)
					{
						m_graphExample->Init();
					}
				}
			}
			ImGui::EndCombo();
		}

		// Print pass dependencies graph.
		ImGui::Text("DEPENDENCIES.");
		ImGui::Spacing();

		if (!graph->SortedPasses.empty())
		{
			PassBase* lastPass = graph->SortedPasses.back();

			if (!graph->PassDependencies(lastPass).empty())
			{
				TraversePassDeps(graph, lastPass);
			}
			else
			{
				ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "None");
			}
		}
		else
		{
			ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "No Graph Selected.");
		}

		ImGui::Spacing();
		ImGui::Spacing();

		// Print pass execution order.
		ImGui::Text("EXECUTION ORDER.");
		ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Start Execution");
		ImGui::Indent();

		uint32_t j = 0;
		for (uint32_t i = 0; i < graph->SortedPasses.size(); ++i)
		{
			ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), graph->SortedPasses[i]->Name().c_str());
			if (i == graph->PassFenceIndices[j])
			{
				if (j < graph->PassFenceIndices.size() - 1) j++;
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "----------Sync----------");
			}
		}

		ImGui::Unindent();
		ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "End Execution");

		ImGui::Spacing();
		ImGui::Spacing();		
		ImGui::Text("FPS: %f", 1.0f / m_window->DeltaTime());
		ImGui::Text("NumLights: %i", GraphExamples::SponzaModel::numLights);
		ImGui::Checkbox("Vsync", &m_window->Context().VSync);
		if (ImGui::Button("Disable ImGui")) m_imguiEnabled = false;
		ImGui::Text("Amount of Transiant resources: %u", graph->ResourceInitializer.NumResources());
		ImGui::Text("Amount of Blackboard resources: %u", m_blackBoard->NumResources());
		ImGui::End();

		tex.Resource()->Transition(*m_cmdList, D3D12_RESOURCE_STATE_GENERIC_READ);
		m_imGUi.EndFrame(*m_cmdList, m_blackBoard->Get<BFTexture>("Screen"));
		
		m_cmdList->EndGPUMarker();
		m_cmdList->Close();
		DX12API()->Queue(QueueType::Direct)->Execute(*m_cmdList);
		DX12API()->Queue(QueueType::Direct)->WaitForFence();
		m_cmdList->Reset();
	}

	void App::OnResize(const Butterfly::WindowResizeEvent& ev)
	{
		BF_PROFILE_EVENT()

		BFTextureDesc desc;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.Width = ev.Width;
		desc.Height = ev.Height;
		desc.Flags = BFTextureDesc::RenderTargettable;
		BFTexture* newComposite = BFTexture::CreateTextureForGPU(desc, "Screen RenderTarget");

		BFTexture* out;
		if (m_blackBoard->TryGet<BFTexture>("Screen", out))
		{
			FREE(out);
		}
		m_blackBoard->RegisterOrReplace<BFTexture>(newComposite, "Screen");
	}

	void App::SetCompositeBufferResolutionIfChanged(uint32_t width, uint32_t height)
	{
		if (CompositeTexture)
		{
			if (CompositeTexture->Width() == width && CompositeTexture->Height() == height)
			{
				return;
			}
		}

		m_graphResources->Flush();

		BFTextureDesc desc;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.Width = width;
		desc.Height = height;
		desc.Flags = BFTextureDesc::RenderTargettable | BFTextureDesc::ShaderResource;
		auto* composite = BFTexture::CreateTextureForGPU(desc, "Composite RenderTarget");

		if (CompositeTexture)
		{
			FREE(CompositeTexture);
		}
		CompositeTexture = composite;

		float aspectRatio = (float)width / (float)height;
		CameraProjection projection = m_spectatorCam.GetCamera()->Projection();
		projection.AspectRatio = aspectRatio;
		m_spectatorCam.GetCamera()->SetProjection(projection);
	}
}
