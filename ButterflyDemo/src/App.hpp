#pragma once
#include "Butterfly.hpp"
#include "Input/Input.hpp"
#include "Tools/ImGUIRenderer.hpp"
#include <string_view>
#include "Tools/SpectatorCamera.hpp"

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
		virtual void OnInit();
		virtual void OnTick();
		virtual void OnShutdown();

	private:
		Input m_input;
		ImGUIRenderer m_imGUi;
		SpectatorCamera m_spectatorCam;

	};
}