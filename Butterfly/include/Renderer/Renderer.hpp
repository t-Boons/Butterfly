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
	struct Viewport
	{
		RefPtr<BFTexture> RenderTarget;
		RefPtr<GraphTransientResourceCache> GraphResources;
		RefPtr<BFUniformBuffer> Uniforms;

		uint32_t UniformCameraDataViewIndex;

		RefPtr<BFStructuredBuffer> ModelMatrices;
	};

	struct FrameData
	{
		RefPtr<BFTexture> RenderTarget;
		RefPtr<D3D12CommandList> CmdList;
		RefPtr<D3D12Fence> Fence;
		uint32_t FrameIndex;
		std::vector<Viewport> Viewports;
	};

	struct FrameCreateData
	{
		glm::ivec2 Size;
	};

	struct ViewportResizeEvent
	{
		glm::ivec2 Size;
	};

	class Renderer : public NonCopyable
	{
	public:
		Renderer();

		void Render();

		~Renderer();

		EventDispatcher<FrameData&> OnImGUIRender;
		EventDispatcher<ViewportResizeEvent> OnViewportResize;

		void ImGUIImage(FrameData& frame, uint32_t viewportIndex);
	private:
		void InvalidateFrameDatas(const FrameCreateData& createData);
		void WaitForInflightFrames();
		void RecordCmdList(FrameData& frameData, uint32_t viewportIndex);
		void ApplyResize();



		std::vector<FrameData> m_frameDatas;

		ScopePtr<Blackboard> m_blackBoard;

		uint32_t m_frameIndex = 0;
		uint32_t m_previousFrame = 0;

		bool m_resizePending = false;
		glm::ivec2 m_resizeSize;

		uint32_t m_numViewports = 1;

		RefPtr<BFTexture> m_whiteTexture;
	};
}