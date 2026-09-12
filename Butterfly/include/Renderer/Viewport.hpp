#pragma once
#include "Core/Common.hpp"
#include "Core/EventDispatcher.hpp"

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
	private:
		friend class Renderer;
		friend struct std::hash<Butterfly::ViewportHandle>;
		uint32_t m_index = 0;
	};


	class BFTexture;
	class BFUniformBuffer;
	class BFStructuredBuffer;
	class GraphTransientResourceCache;

	struct Viewport
	{
	public:
		RefPtr<BFTexture> RenderTarget;
		RefPtr<BFUniformBuffer> Uniforms;

		RefPtr<BFStructuredBuffer> ModelMatrices;
	private:
		friend class Renderer;
		RefPtr<GraphTransientResourceCache> GraphResources;
		ViewportHandle Handle;
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

	struct ViewportEvents
	{
		EventDispatcher<ViewportResizeEvent> OnResize;
		EventDispatcher<ViewportPrerenderEvent> OnPreRender;
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