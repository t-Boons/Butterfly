#pragma once
#include "Butterfly.hpp"
#include "Input/Input.hpp"
#include "Tools/ImGUIRenderer.hpp"
#include <string_view>
#include "Tools/SpectatorCamera.hpp"

#define NUM_RENDER_BUFFERS 3

namespace Butterfly
{
	struct FrameData
	{
		RefPtr<BFTexture> RenderTarget;
		RefPtr<GraphTransientResourceCache> GraphResources;
		RefPtr<BFUniformBuffer> Uniforms;
		RefPtr<D3D12CommandList> CmdList;
		RefPtr<D3D12Fence> Fence;
		uint32_t FrameIndex;
		bool FramePresentable = false;

		uint32_t UniformCameraDataViewIndex;
	};

	struct FrameCreateData
	{
		glm::ivec2 Size;
	};

	class SandboxLayer : public Butterfly::ApplicationLayer
	{
	public:
		void InvalidateFrameDatas(const FrameCreateData& createData)
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


				m_frameDatas[i].UniformCameraDataViewIndex = m_frameDatas[i].Uniforms->AllocView(sizeof(glm::mat4) * 2);
			}
		}

		void WaitForInflightFrames()
		{
			for (auto& frameData : m_frameDatas)
			{
				D3D12API()->Queue(QueueType::Direct)->WaitForFence();
			}
		}

		void RecordNewFrame(FrameData& frameData)
		{
			// Upload camera data uniform.
			struct UniformCameraData
			{
				glm::mat4 ViewProjection;
				glm::mat4 Model;
			} cameraData;

			cameraData.Model = glm::mat4(1.0f);
			cameraData.ViewProjection = m_spectatorCam.GetCamera()->ViewProjectionMatrix();

			frameData.Uniforms->Write(&cameraData, sizeof(UniformCameraData), frameData.UniformCameraDataViewIndex);

			
			RecordCmdList(frameData);
		}

		virtual void OnInit();
		virtual void OnTick();
		virtual void OnShutdown();
		void RecordCmdList(FrameData& frameData);
		void OnResize(const Butterfly::WindowResizeEvent& ev);
		void OnImGUIResize(uint32_t width, uint32_t height);

		inline static Window* StaticWindow;
	private:
		Input m_input;
		ScopePtr<Window> m_window;

		std::vector<FrameData> m_frameDatas;
		ImGUIRenderer m_imGUi;
		glm::vec2 m_oldWindowSize;
		uint32_t m_frameIndex = 0;
		uint32_t m_frame = 0;
		uint32_t m_previousFrame = 0;

		bool m_imguiEnabled = false;

		ScopePtr<Blackboard> m_blackBoard;

		ScopePtr<BFIndexBuffer> m_modelIndices;
		ScopePtr<BFStructuredBuffer> m_modelPositions;
		ScopePtr<BFStructuredBuffer> m_modelNormals;
		ScopePtr<BFStructuredBuffer> m_modelUVS;
		RefPtr<BFTexture> m_modelAlbedo;

		SpectatorCamera m_spectatorCam;

	};
}