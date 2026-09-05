#pragma once
#include "Core/Common.hpp"
#include "Renderer/D3D12/D3D12Common.hpp"
#include "Renderer/D3D12Texture.hpp"
#include "Renderer/D3d12Buffer.hpp"
#include "Renderer/Graph/Graph.hpp"
#include "Renderer/D3D12/D3D12Fence.hpp"
#include "Renderer/Graph/Blackboard.hpp"
#include "Core/Window.hpp"

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


	class Renderer : public NonCopyable
	{
	public:
		void Init();
		void Render();

	private:
		void InvalidateFrameDatas(const FrameCreateData& createData);
		void WaitForInflightFrames();
		void RecordNewFrame(FrameData& frameData);
		void RecordCmdList(FrameData& frameData);
		void ApplyResize();

		std::vector<FrameData> m_frameDatas;

		ScopePtr<Blackboard> m_blackBoard;

		ScopePtr<BFIndexBuffer> m_modelIndices;
		ScopePtr<BFStructuredBuffer> m_modelPositions;
		ScopePtr<BFStructuredBuffer> m_modelNormals;
		ScopePtr<BFStructuredBuffer> m_modelUVS;
		RefPtr<BFTexture> m_modelAlbedo;

		uint32_t m_frameIndex = 0;
		uint32_t m_previousFrame = 0;

		bool m_resizePending = false;
		glm::ivec2 m_resizeSize;
	};
}