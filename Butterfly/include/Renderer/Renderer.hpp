#pragma once
#include "Core/Common.hpp"
#include "Core/EventDispatcher.hpp"
#include "Core/Window.hpp"
#include "Renderer/RenderIncludes.hpp"

#define NUM_RENDER_BUFFERS 2

namespace Butterfly
{
	class BFRenderData;
	class GraphTransientResourceCache;
	class RenderPipeline;
	class GraphBuilder;
	class Viewport;
	class RenderPipeline;
	class BFRenderData;

	class ViewportHandle
	{
	public:
		bool Valid() const { return m_index != 0; }

		bool operator==(const ViewportHandle& other) const
		{
			return m_index == other.m_index;
		}

		uint32_t Index() const { return m_index; }
	private:
		friend class Renderer;
		friend struct std::hash<Butterfly::ViewportHandle>;
		uint32_t m_index = 0;
	};

	struct ViewportResizeEvent
	{
		glm::ivec2 Size;
	};

	struct ViewportRenderEvent
	{
		GraphBuilder& Builder;
		Viewport& Viewport;
	};

	struct ViewportPrerenderEvent
	{
		Viewport& Viewport;
	};

	struct ViewportEvents
	{
		EventDispatcher<ViewportResizeEvent> OnResize;
		EventDispatcher<ViewportPrerenderEvent> OnPreRender;
		EventDispatcher<> OnPostRender;
		EventDispatcher<ViewportRenderEvent> OnRender;
	};
}

namespace std
{
	template<>
	struct hash<Butterfly::ViewportHandle>
	{
		size_t operator()(const Butterfly::ViewportHandle& handle) const
		{
			return std::hash<uint32_t>()(handle.m_index);
		}
	};
}

namespace Butterfly
{
	struct Viewport
	{
	public:
		glm::ivec2 Size() const { return { GetRenderTarget().Width(), GetRenderTarget().Height() }; }

		bool HasRenderTarget() const { return m_renderTarget[m_frameIndex] != nullptr; }
		BFTexture& GetRenderTarget() const { return *m_renderTarget[m_frameIndex]; }
		BFTexture& GetDepthStencil() const { return *m_depthStencil[m_frameIndex]; }
		GraphTransientResourceCache& GetGraphResources() const { return *m_graphResources[m_frameIndex]; }
		ViewportEvents& GetEvents() { return m_events; }
		RenderPipeline& GetRenderPipeline() { return *m_renderPipeline; }
		const RenderPipeline& GetRenderPipeline() const { return *m_renderPipeline; }
		BFRenderData& GetRenderData() { return *m_renderData; }
		const BFRenderData& GetRenderData() const { return *m_renderData; }
	private:
		friend class Renderer;

		RefPtr<BFRenderData> m_renderData;
		RefPtr<RenderPipeline> m_renderPipeline;
		ViewportEvents m_events;
		std::array<RefPtr<BFTexture>, NUM_RENDER_BUFFERS> m_renderTarget;
		std::array<RefPtr<BFTexture>, NUM_RENDER_BUFFERS> m_depthStencil;
		std::array<RefPtr<GraphTransientResourceCache>, NUM_RENDER_BUFFERS> m_graphResources;

		ViewportHandle m_handle;
		uint32_t m_frameIndex;
	};

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
		std::unordered_map<ViewportHandle, Viewport>& GetViewports() { return m_frameData.Viewports; }

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