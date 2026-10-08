#pragma once
#include "Core/Common.hpp"
#include "Renderer/RenderIncludes.hpp"
#include "Renderer/Viewport.hpp"
#include "Core/Window.hpp"
#include "Renderer/Light.hpp"
#include "Renderer/Material.hpp"
#include "Renderer/Model.hpp"

namespace Butterfly
{
	class D3D12Fence;
	class D3D12CommandList;
	class BFTexture;
	class BFSampler;
	class RenderPipeline;

	struct FrameData
	{
		BFTexture& GetCompositeRenderTarget() const { return *CompositeRenderTarget[FrameIndex]; }
		D3D12CommandList& GetCmdList() const { return *CmdList[FrameIndex]; }
		D3D12Fence& GetFence() const { return *Fence[FrameIndex]; }

		std::unordered_map<ViewportHandle, Viewport> Viewports;
	private:
		friend class Renderer;
		uint32_t FrameIndex;
		std::array<RefPtr<BFTexture>, NUM_RENDER_BUFFERS> CompositeRenderTarget;
		std::array<RefPtr<D3D12CommandList>, NUM_RENDER_BUFFERS> CmdList;
		std::array<RefPtr<D3D12Fence>, NUM_RENDER_BUFFERS> Fence;

	};

	struct CameraData
	{
		glm::mat4 ViewProjection;
		glm::vec3 CameraPosition;
	};

	struct InverseCameraData
	{
		glm::mat4 InverseView;
		glm::mat4 InverseProjection;
	};

	class BFRGTexture;
	struct ForwardRenderer
	{
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

		FrameData& GetFrameData() { return m_frameData; }
		const FrameData& GetFrameData() const { return m_frameData; }
		const Viewport& GetViewport(const ViewportHandle& handle) const;
		Viewport& GetViewport(const ViewportHandle& handle);

		const std::unordered_map<ViewportHandle, Viewport>& GetViewports() const { return m_frameData.Viewports; }

		// ImGui Helper functions.
		EventDispatcher<>& GetImGUIRenderEvent() { return m_ImGuiRenderEvent; }
		void ImGUIImage(const ViewportHandle& handle);

	private:
		void WaitForInflightFrames();
		void ApplyResize();

		void RecordCmdList(const ViewportRenderEvent& ev);
		void OnWindowResize(const WindowResizeEvent& ev);
		void OnWindowRefresh();

		uint32_t m_frameIndex = 0;
		uint32_t m_previousFrame = 0;
		FrameData m_frameData;

		bool m_resizePending = false;
		glm::ivec2 m_resizeSize;

		uint32_t m_viewportHandleIndex = 1;

		EventDispatcher<> m_ImGuiRenderEvent;

		EventReceiver<WindowResizeEvent> m_windowResizeReceiver;
		EventReceiver<> m_windowRefreshReceiver;
	};
}