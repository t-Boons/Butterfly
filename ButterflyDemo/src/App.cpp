#include "App.hpp"
#include "glm/gtx/quaternion.hpp"
#include <numeric>

namespace Butterfly
{
	void App::Init()
	{
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

		D3D12API()->DescriptorAllocatorSrvCbvUav()->AllocateDummy(); // Because ImGUI takes slot 0;


		SetCompositeBufferResolutionIfChanged(m_window->Width(), m_window->Height());

		WindowResizeEvent ev;
		ev.Width = m_window->Width();
		ev.Height = m_window->Height();
		OnResize(ev);

		// Create command list.
		auto genericCmdList = Graphics->CreateCommandList(QueueType::Direct);
		m_cmdList = std::dynamic_pointer_cast<D3D12CommandList>(genericCmdList);


		m_currentDemo = m_demos[0];
		m_spectatorCam.GetCamera()->SetPosition({ 0.0f, 1.0f, 0.0f });


		// Load the test model.
		RefPtr<ModelImporter> importer = ModelImporter::Create("assets/Models/damagedhelmet/DamagedHelmet.gltf");
		importer->Load();

		auto& material = importer->Materials()[0];

		// Model texture(s)
		BFTextureDesc desc;
		desc.Flags = BFTextureDesc::ShaderResource;
		desc.Width = material->m_colorTexture->m_width;
		desc.Height = material->m_colorTexture->m_height;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;


		m_modelAlbedo = ScopePtr<BFTexture>(BFTexture::CreateTextureFromCPUBuffer(
			desc,
			material->m_colorTexture->m_image.data(),
			material->m_name));

		// Model Indices
		auto& mesh = importer->Meshes()[0];

		m_modelIndices = ScopePtr<BFIndexBuffer>(new BFIndexBuffer(mesh->m_indices[0].data(), static_cast<uint32_t>(mesh->m_indices[0].size()), DXGI_FORMAT_R32_UINT, "ModelIndices"));

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		{
			srvDesc.Buffer.StructureByteStride = sizeof(glm::vec3);
			srvDesc.Buffer.NumElements = static_cast<uint32_t>(mesh->m_positions[0].size());
			const auto size = static_cast<uint32_t>(mesh->m_positions[0].size() * sizeof(glm::vec3));
			m_modelPositions = ScopePtr<BFStructuredBuffer>(new BFStructuredBuffer(mesh->m_positions[0].data(), size, &srvDesc, "Position"));
		}

		{
			srvDesc.Buffer.StructureByteStride = sizeof(glm::vec3);
			srvDesc.Buffer.NumElements = static_cast<uint32_t>(mesh->m_normals[0].size());
			const auto size = static_cast<uint32_t>(mesh->m_normals[0].size() * sizeof(glm::vec3));
			m_modelNormals = ScopePtr<BFStructuredBuffer>(new BFStructuredBuffer(mesh->m_normals[0].data(), size, &srvDesc, "Normals"));
		}
		{
			srvDesc.Buffer.StructureByteStride = sizeof(glm::vec2);
			srvDesc.Buffer.NumElements = static_cast<uint32_t>(mesh->m_texcoords[0].size());
			const auto size = static_cast<uint32_t>(mesh->m_texcoords[0].size() * sizeof(glm::vec2));
			m_modelUVS = ScopePtr<BFStructuredBuffer>(new BFStructuredBuffer(mesh->m_texcoords[0].data(), size, &srvDesc, "TexCoords"));
		}


		m_uniforms = ScopePtr<BFUniformBuffer>(new BFUniformBuffer(256, "Uniforms"));
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
		D3D12API()->Queue(QueueType::Direct)->Execute(*m_cmdList);
		D3D12API()->Queue(QueueType::Direct)->WaitForFence();
		m_cmdList->Reset();

		GraphBuilder builder(*m_graphResources);

		using namespace Butterfly;

		struct UniformData
		{
			glm::mat4 ViewProjection;
			glm::mat4 Model;
		};

		UniformData uniformData;
		uniformData.ViewProjection = m_spectatorCam.GetCamera()->ViewProjectionMatrix();
		uniformData.Model = glm::mat4(glm::quat(glm::vec3(glm::radians(90.0f), 0.0f, 0.0f)));

		m_uniforms->Write(&uniformData, sizeof(uniformData));


		struct ForwardRenderer
		{
			BFRGTexture* DepthStencil;
		};

		ForwardRenderer* params = builder.AllocParameters<ForwardRenderer>();

		BFTextureDesc desc2;
		desc2.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		desc2.Width = App::CompositeTexture->Width();
		desc2.Height = App::CompositeTexture->Height();
		desc2.Flags = BFTextureDesc::DepthStencilable;
		params->DepthStencil = builder.CreateTransientTexture("DepthStencil Positions", desc2);

		builder.AddPass<ForwardRenderer>("Forward Model",
			[&](const ForwardRenderer& params, D3D12CommandList& list)
			{
				BF_PROFILE_EVENT_DYNAMIC("Forward Model pass");

				BFTexture& rt = *App::CompositeTexture;

				// Default Init stuff.
				list.List()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				GraphicsCommands::SetRenderTargets(list, { &rt }, params.DepthStencil->Resource());

				GraphicsCommands::ClearDepthStencil(list, *params.DepthStencil->Resource());
				GraphicsCommands::ClearRenderTarget(list, rt, { 0.05f, 0.1f, 0.15f, 1.0f });

				GraphicsCommands::SetFullscreenViewportAndRect(list, App::CompositeTexture->Width(), App::CompositeTexture->Height());

				BFPipelineBuilder psoBuilder;
				psoBuilder.PrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
				psoBuilder.RenderTargetFormats({ DXGI_FORMAT_R8G8B8A8_UNORM });
				psoBuilder.DepthStencilFormat({ DXGI_FORMAT_D24_UNORM_S8_UINT });
				psoBuilder.VertexShader(BFShaderCache::GetOrCreate(L"assets/Shaders/Forward_vert.hlsl", ShaderType::Vertex));
				psoBuilder.PixelShader(BFShaderCache::GetOrCreate(L"assets/Shaders/Forward_frag.hlsl", ShaderType::Pixel));
				psoBuilder.CullingMode(D3D12_CULL_MODE_FRONT);

				list.List()->SetPipelineState(psoBuilder.Create().GetHW());


				BFSampler sampler;

				ShaderVariables()
					.Add(m_modelPositions->SRV().View())
					.Add(m_modelNormals->SRV().View())
					.Add(m_modelUVS->SRV().View())
					.Add(m_uniforms->CBV().View())
					.Add(sampler.View())
					.Add(m_modelAlbedo->SRV().View())
					.Submit(list);

				list.List()->IASetIndexBuffer(&m_modelIndices->IBV());
				list.List()->DrawIndexedInstanced(m_modelIndices->NumElements(), 1, 0, 0, 0);
			});
	

		auto graph = builder.Create();

		graph->Execute();

		if (m_imguiEnabled)
		{
			ImGuiRender(graph);
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

		delete graph;
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

		m_imGUi.BeginFrame();
		ImGui::DockSpaceOverViewport();
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Butterfly Demo", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground);
		ImVec2 newSize = ImGui::GetContentRegionAvail();
		SetCompositeBufferResolutionIfChanged(static_cast<uint32_t>(newSize.x), static_cast<uint32_t>(newSize.y));
		BFTexture& tex = *CompositeTexture;
		ImTextureID textureID = (ImTextureID)(uintptr_t)D3D12API()->DescriptorAllocatorSrvCbvUav()->GpuHandleFromSrvHandle(tex.SRV().View()).ptr;
		ImGui::Image(textureID, newSize);
		ImGui::End();
		ImGui::PopStyleVar();

		tex.Resource()->Transition(*m_cmdList, D3D12_RESOURCE_STATE_GENERIC_READ);
		m_imGUi.EndFrame(*m_cmdList, m_blackBoard->Get<BFTexture>("Screen"));

		m_cmdList->EndGPUMarker();
		m_cmdList->Close();
		D3D12API()->Queue(QueueType::Direct)->Execute(*m_cmdList);
		D3D12API()->Queue(QueueType::Direct)->WaitForFence();
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
		desc.DebugName = "Screen RenderTarget";
		BFTexture* newComposite = BFTexture::CreateTextureForGPU(desc);

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
		desc.DebugName = "Composite RenderTarget";
		auto* composite = BFTexture::CreateTextureForGPU(desc);

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
