#pragma once
#include "Core/Common.hpp"
#include "Core/EventDispatcher.hpp"

#include "Renderer/D3D12Texture.hpp"


#define NUM_RENDER_BUFFERS 3

namespace Butterfly
{
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

	class GraphBuilder;
	class Viewport;
	struct ViewportRenderEvent
	{
		GraphBuilder& Builder;
		Viewport& Viewport;
	};

	struct ViewportPrerenderEvent
	{
		Viewport& Viewport;
	};

	struct ViewportPostRenderEvent
	{
		GraphBuilder& Builder;
		Viewport& Viewport;
	};

	struct ViewportEvents
	{
		EventDispatcher<ViewportResizeEvent> OnResize;
		EventDispatcher<ViewportPrerenderEvent> OnPreRender;
		EventDispatcher<ViewportPostRenderEvent> OnPostRender;
		EventDispatcher<ViewportRenderEvent> OnRender;
	};


	class BFTexture;
	class BFUniformBuffer;
	class BFStructuredBuffer;
	class GraphTransientResourceCache;
	class LightBuffer;
	class MaterialLibrary;
	class RenderPipeline;
	class ModelBuffer;
	struct Viewport
	{
	public:
		glm::ivec2 Size() const { return { GetRenderTarget().Width(), GetRenderTarget().Height()}; }

		bool HasRenderTarget() const { return RenderTarget[FrameIndex] != nullptr; }
		BFTexture& GetRenderTarget() const { return *RenderTarget[FrameIndex]; }
		BFTexture& GetDepthStencil() const { return *DepthStencil[FrameIndex]; }
		GraphTransientResourceCache& GetGraphResources() const { return *GraphResources[FrameIndex]; }

		RefPtr<BFUniformBuffer> Uniforms;
		RefPtr<MaterialLibrary> Materials;
		RefPtr<LightBuffer> Lights;
		RefPtr<ModelBuffer> Models;
		RefPtr<RenderPipeline> RenderPipeline;

		ViewportEvents Events;
		bool ShouldRender = true;

	private:
		friend class Renderer;

		std::array<RefPtr<BFTexture>, NUM_RENDER_BUFFERS> RenderTarget;
		std::array<RefPtr<BFTexture>, NUM_RENDER_BUFFERS> DepthStencil;
		std::array<RefPtr<GraphTransientResourceCache>, NUM_RENDER_BUFFERS> GraphResources;

		ViewportHandle Handle;
		uint32_t FrameIndex;
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