#pragma once
#include "Core/Common.hpp"
#include "Renderer/Viewport.hpp"
#include "Core/Window.hpp"

#define NUM_RENDER_BUFFERS 3

namespace Butterfly
{
	class D3D12Fence;
	class D3D12CommandList;
	class BFTexture;

	struct FrameData
	{
		RefPtr<BFTexture> CompositeRenderTarget;
		RefPtr<D3D12CommandList> CmdList;
		RefPtr<D3D12Fence> Fence;
		uint32_t FrameIndex;
		std::unordered_map<ViewportHandle, Viewport> Viewports;
	};

	struct CameraData
	{
		glm::mat4 ViewProjection;
	};

	struct InverseCameraData
	{
		glm::mat4 InverseView;
		glm::mat4 InverseProjection;
	};

	class BFRGTexture;
	struct ForwardRenderer
	{
		BFRGTexture* DepthStencil;
		BFTexture* Comp;
	};

	class Skybox;
	class Renderer : public NonCopyable
	{
	public:
		Renderer();
		~Renderer();

		void Render();

		// Viewport functions.
		ViewportHandle AddViewport();
		void RemoveViewport(const ViewportHandle& handle);
		ViewportEvents& GetViewportEvents(const ViewportHandle& handle);

		FrameData& GetCurrentFrameData() { return m_frameDatas[m_frameIndex]; }
		const Viewport& GetViewport(const ViewportHandle& handle);

		// ImGui Helper functions.
		EventDispatcher<>& GetImGUIRenderEvent() { return m_ImGuiRenderEvent; }
		EventDispatcher<>& GetRenderFinishedEvent() { return m_renderFinishedEvent; }
		void ImGUIImage(const ViewportHandle& handle);

		RefPtr<Skybox> m_tempSkybox;
	private:
		void InvalidateFrameDatas();
		void WaitForInflightFrames();
		void ApplyResize();

		void RecordCmdList(const ViewportRenderEvent& ev);
		void OnWindowResize(const WindowResizeEvent& ev);
		void OnWindowRefresh();

		RefPtr<BFTexture> m_whiteTexture;

		uint32_t m_frameIndex = 0;
		uint32_t m_previousFrame = 0;

		std::vector<FrameData> m_frameDatas;


		bool m_resizePending = false;
		glm::ivec2 m_resizeSize;


		std::vector<ViewportHandle> m_existingViewportHandles;
		std::unordered_map<ViewportHandle, ViewportEvents> m_viewportEvents;
		uint32_t m_viewportHandleIndex = 1;

		EventDispatcher<> m_ImGuiRenderEvent;
		EventDispatcher<> m_renderFinishedEvent;

		EventReceiver<WindowResizeEvent> m_windowResizeReceiver;
		EventReceiver<> m_windowRefreshReceiver;
	};
}