#include "App.hpp"
#include "glm/gtx/quaternion.hpp"
#include <numeric>

namespace Butterfly
{
	void SandboxLayer::OnInit()
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
			m_window->Events().OnWindowResize.Subscribe(BF_BIND_FUNC(&SandboxLayer::OnResize));
			m_imGUi.Init(m_window.get());

			StaticWindow = m_window.get();
			m_input.Init(m_window.get());
		}

		// Create resouce cache.
		m_blackBoard = ScopePtr<Blackboard>(new Blackboard());

		D3D12API()->DescriptorAllocatorSrvCbvUav()->AllocateDummy(); // Because ImGUI takes slot 0;

		WindowResizeEvent ev;
		ev.Width = m_window->Width();
		ev.Height = m_window->Height();
		OnResize(ev);

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


		m_modelAlbedo = BFTexture::CreateTextureFromCPUBuffer(
			desc,
			material->m_colorTexture->m_image.data(),
			material->m_name);

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
	}

	inline float ms_since(std::chrono::high_resolution_clock::time_point start)
	{
		return std::chrono::duration<float, std::milli>(
			std::chrono::high_resolution_clock::now() - start).count();
	}

	void SandboxLayer::OnTick()
	{
		BF_PROFILE_FRAME("SandboxLayer::OnTick");
		m_spectatorCam.Tick(m_input, m_window->DeltaTime());
		if (m_window->ShouldClose())
		{
			m_app->Quit();
		}

		if (m_input.IsKeyDown(BFB_F))
		{
			//m_graphResources->Flush();
			m_imguiEnabled = !m_imguiEnabled;
		}

		if (m_input.IsKeyDown(BFB_F11))
		{
			m_window->SetFullscreen(!m_window->Fullscreen());
		}

		FrameData& frame = m_frameDatas[m_frameIndex];
		frame.FrameIndex = m_frameIndex;
		frame.Fence->Wait();
		frame.CmdList->Reset();
		m_window->Context().DisplayTexture(*frame.RenderTarget->Resource());
		m_window->Update();

		RecordNewFrame(frame);

		D3D12API()->Queue(QueueType::Direct)->Execute(*frame.CmdList);
		frame.Fence->Signal(*D3D12API()->Queue(QueueType::Direct));

		m_window->Update();
		m_input.Poll();
		m_previousFrame = m_frameIndex;
		m_frameIndex++;
		m_frameIndex = m_frameIndex % NUM_RENDER_BUFFERS;
	}

	void SandboxLayer::RecordCmdList(FrameData& frameData)
	{
		frameData.CmdList->BeginGPUMarker("Render Frame -> " + std::to_string(frameData.FrameIndex));

		frameData.CmdList->BeginGPUMarker("Composite Clear.");
		GraphicsCommands::ClearRenderTarget(*frameData.CmdList, *frameData.RenderTarget, { 0.0, 0.05f, 0.1f, 1.0f });
		frameData.CmdList->EndGPUMarker();

		GraphBuilder builder(*frameData.GraphResources);

		struct ForwardRenderer
		{
			BFRGTexture* DepthStencil;
			BFTexture* Comp;
		};

		ForwardRenderer* params = builder.AllocParameters<ForwardRenderer>();

		params->Comp = frameData.RenderTarget.get();


		BFTextureDesc desc2;
		desc2.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		desc2.Width = frameData.RenderTarget->Width();
		desc2.Height = frameData.RenderTarget->Height();
		desc2.Flags = BFTextureDesc::DepthStencilable;
		params->DepthStencil = builder.CreateTransientTexture("DepthStencil Positions", desc2);

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


				BFSampler sampler;

				ShaderVariables()
					.Add(m_modelPositions->SRV().View())
					.Add(m_modelNormals->SRV().View())
					.Add(m_modelUVS->SRV().View())
					.Add(frameData.Uniforms->GetView(frameData.UniformCameraDataViewIndex)->View())
					.Add(sampler.View())
					.Add(m_modelAlbedo->SRV().View())
					.Submit(list);

				list.List()->IASetIndexBuffer(&m_modelIndices->IBV());
				list.List()->DrawIndexedInstanced(m_modelIndices->NumElements(), 1, 0, 0, 0);
			});


		auto graph = builder.Create();
		graph->Execute(*frameData.CmdList);
		delete graph;


		frameData.CmdList->EndGPUMarker();
		frameData.CmdList->Close();
	}

	void SandboxLayer::OnShutdown()
	{
		BF_PROFILE_EVENT()
		//m_graphResources->Flush();

		FullscreenQuad::ShutDown();
	}

	void SandboxLayer::OnResize(const WindowResizeEvent& ev)
	{
		BF_PROFILE_EVENT()
		
		FrameCreateData createData;
		createData.Size = { ev.Width, ev.Height };
		InvalidateFrameDatas(createData);
	}

	void SandboxLayer::OnImGUIResize(uint32_t width, uint32_t height)
	{
		BF_PROFILE_EVENT()

		//m_graphResources->Flush();

		BFTextureDesc desc;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.Width = width;
		desc.Height = height;
		desc.Flags = BFTextureDesc::RenderTargettable | BFTextureDesc::ShaderResource;
		desc.DebugName = "ImGUi Screen RenderTarget";

		RefPtr<BFTexture> newScreen = BFTexture::CreateTextureForGPU(desc);

		RefPtr<BFTexture> oldScreen;

		if (m_blackBoard->TryGet<BFTexture>("ImGuiScreen", oldScreen))
		{
			oldScreen.reset();
		}

		m_blackBoard->RegisterOrReplace<BFTexture>(newScreen, "ImGuiScreen");

		//ImGUITexture = newScreen;
	}
}
