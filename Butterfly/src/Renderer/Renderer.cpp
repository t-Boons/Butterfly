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

#include "Scene/Scene.hpp"
#include "Scene/Registry/MeshRenderer.hpp"
#include "Scene/Registry/Transform.hpp"

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

		RecordNewFrame(frame);
		Application::Get().GetWindow().Context().RecordCopyToBackBuffer(*frame.RenderTarget->Resource(), *frame.CmdList);

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
			m_frameDatas[i].GraphResources = MakeRef<GraphTransientResourceCache>();
			m_frameDatas[i].Uniforms = MakeRef<BFUniformBuffer>(4096, "Frame " + std::to_string(i) + " Uniforms");
			m_frameDatas[i].CmdList = MakeRef<D3D12CommandList>();
			m_frameDatas[i].Fence = MakeRef<D3D12Fence>();
			m_frameDatas[i].FramePresentable = false;


			m_frameDatas[i].UniformCameraDataViewIndex = m_frameDatas[i].Uniforms->AllocView(sizeof(UniformCameraData));
		}
	}

	void Renderer::WaitForInflightFrames()
	{
		for (auto& frameData : m_frameDatas)
		{
			frameData.Fence->SignalAndWait(*D3D12API()->Queue(QueueType::Direct));
		}
	}

	void Renderer::RecordNewFrame(FrameData& frameData)
	{
		// Upload camera data uniform.

		//UniformCameraData cameraData;
		//cameraData.Model = glm::mat4(1.0f);
		//cameraData.ViewProjection = Application::Get().GetBlackboard().Get<Camera>("ViewCamera")->ViewProjectionMatrix();
		//
		//frameData.Uniforms->Write(&cameraData, sizeof(UniformCameraData), frameData.UniformCameraDataViewIndex);


		RecordCmdList(frameData);
	}


	void Renderer::RecordCmdList(FrameData& frameData)
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
				auto view = Application::Get().GetScene().GetEntityRegistry().view<Transform, MeshRenderer>();
				for (auto [entity, transform, meshRenderer] : view.each())
				{
					UniformCameraData cameraData;
					cameraData.Model = transform.GetMatrix();
					cameraData.ViewProjection = Application::Get().GetBlackboard().Get<Camera>("ViewCamera")->ViewProjectionMatrix();

					frameData.Uniforms->Write(&cameraData, sizeof(UniformCameraData), frameData.UniformCameraDataViewIndex);

					ShaderVariables()
						.Add(meshRenderer.m_modelPositions->SRV().View())
						.Add(meshRenderer.m_modelNormals->SRV().View())
						.Add(meshRenderer.m_modelUVS->SRV().View())
						.Add(frameData.Uniforms->GetView(frameData.UniformCameraDataViewIndex)->View())
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


		frameData.CmdList->EndGPUMarker();
	}

	void Renderer::ApplyResize()
	{
		m_resizePending = false;

		FrameCreateData createData;
		createData.Size = m_resizeSize;

		InvalidateFrameDatas(createData);
	}
}