#pragma once
#include "Core/Common.hpp"
#include "Renderer/D3D12/D3D12Common.hpp"
#include "Renderer/D3D12Texture.hpp"
#include "Renderer/D3d12Buffer.hpp"
#include "Renderer/Graph/Graph.hpp"
#include "Renderer/D3D12/D3D12Fence.hpp"
#include "Renderer/Graph/Blackboard.hpp"
#include "Core/Window.hpp"
#include "Core/EventDispatcher.hpp"


#define NUM_RENDER_BUFFERS 3

namespace Butterfly
{
	class GraphBuilder;
	class Viewport;
	struct RecordRenderPassEvent
	{
		GraphBuilder& Builder;
		Viewport& Viewport;
	};


	struct ViewportHandle
	{
	public:
		bool Valid() const { return Index != 0; }
	private:
		friend class Renderer;
		uint32_t Index = 0;
	};

	struct Viewport
	{
	public:
		RefPtr<BFTexture> RenderTarget;
		RefPtr<BFUniformBuffer> Uniforms;

		uint32_t UniformCameraDataViewIndex;
		RefPtr<BFStructuredBuffer> ModelMatrices;
		
	private:
		friend class Renderer;
		RefPtr<GraphTransientResourceCache> GraphResources;
		ViewportHandle Handle;
	};

	struct FrameData
	{
		RefPtr<BFTexture> CompositeRenderTarget;
		RefPtr<D3D12CommandList> CmdList;
		RefPtr<D3D12Fence> Fence;
		uint32_t FrameIndex;
		std::unordered_map<uint32_t, Viewport> Viewports;
	};

	struct ViewportResizeEvent
	{
		glm::ivec2 Size;
	};

	struct ViewportEvents
	{
		EventDispatcher<ViewportResizeEvent> OnResize;
		EventDispatcher<RecordRenderPassEvent> OnRender;
	};

	class Renderer : public NonCopyable
	{
	public:
		Renderer();
		~Renderer();

		void Render();

		ViewportHandle AddViewport();
		void RemoveViewport(const ViewportHandle& handle);
		ViewportEvents& GetViewportEvents(const ViewportHandle& handle);

		FrameData& CurrentFrameData() { return m_frameDatas[m_frameIndex]; }

		EventDispatcher<>& GetImGUIRenderEvent() { return m_ImGuiRenderEvent; }
		void ImGUIImage(const ViewportHandle& handle);

	private:
		void InvalidateFrameDatas();
		void WaitForInflightFrames();
		void ApplyResize();

		void RecordCmdList(const RecordRenderPassEvent& ev);
		void OnWindowResize(const WindowResizeEvent& ev);
		void OnWindowRefresh();

		RefPtr<BFTexture> m_whiteTexture;

		uint32_t m_frameIndex = 0;
		uint32_t m_previousFrame = 0;

		std::vector<FrameData> m_frameDatas;


		bool m_resizePending = false;
		glm::ivec2 m_resizeSize;


		std::vector<ViewportHandle> m_existingViewportHandles;
		std::unordered_map<uint32_t, ViewportEvents> m_viewportEvents;
		uint32_t m_index = 1;

		EventDispatcher<> m_ImGuiRenderEvent;
	};
}