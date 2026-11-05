#pragma once
#include "Core/Common.hpp"
#include "Core/EventDispatcher.hpp"
#include "Core/Window.hpp"
#include "Renderer/RenderIncludes.hpp"

#define NUM_RENDER_BUFFERS 3

namespace Butterfly
{
	class BFRenderData;
	class GraphTransientResourceCache;
	class RenderPipeline;
	class GraphBuilder;
	class Viewport;
	class RenderPipeline;
	class BFRenderData;

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
		const std::string& GetName() const { return m_viewportName; }
		uint32_t GetFrameIndex() const { return m_frameIndex; }

	private:
		friend class Renderer;

		RefPtr<BFRenderData> m_renderData;
		RefPtr<RenderPipeline> m_renderPipeline;
		ViewportEvents m_events;
		std::array<RefPtr<BFTexture>, NUM_RENDER_BUFFERS> m_renderTarget;
		std::array<RefPtr<BFTexture>, NUM_RENDER_BUFFERS> m_depthStencil;
		std::array<RefPtr<GraphTransientResourceCache>, NUM_RENDER_BUFFERS> m_graphResources;

		uint32_t m_frameIndex;
		std::string m_viewportName;
	};

	struct FrameData
	{
		BFTexture& GetCompositeRenderTarget() const { return *CompositeRenderTarget[FrameIndex]; }
		D3D12CommandList& GetCmdList() const { return *CmdList[FrameIndex]; }
		D3D12Fence& GetFence() const { return *Fence[FrameIndex]; }

		std::unordered_set<RefPtr<Viewport>> Viewports;
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

		RefPtr<Viewport> CreateViewport(const std::string& name);

		FrameData& GetFrameData() { return m_frameData; }
		const FrameData& GetFrameData() const { return m_frameData; }

		// ImGui Helper functions.
		EventDispatcher<>& GetImGUIRenderEvent() { return m_ImGuiRenderEvent; }
		void ImGUIImage(const RefPtr<Viewport>& viewport);

		const std::unordered_set<RefPtr<Viewport>>& GetAllViewports() const { return m_frameData.Viewports; }
		std::unordered_set<RefPtr<Viewport>>& GetAllViewports() { return m_frameData.Viewports; }

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

		EventDispatcher<> m_ImGuiRenderEvent;

		EventReceiver<WindowResizeEvent> m_windowResizeReceiver;
		EventReceiver<> m_windowRefreshReceiver;
	};
}