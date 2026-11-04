#pragma once
#include "Butterfly.hpp"

namespace Butterfly::Game
{
	class GameViewport
	{
	public:
		GameViewport();

		void OnRenderImGUI();

		void OnPrerender(const ViewportPrerenderEvent& event);
		void RenderObjectPicker(const ViewportRenderEvent& event);

	private:
		RefPtr<Viewport> m_viewport;
		uint32_t m_mainCameraIndex = 0;

		EventReceiver<ViewportResizeEvent> m_viewportResizeReceiver;
		EventReceiver<ViewportPrerenderEvent> m_viewportPrerenderReceiver;
		EventReceiver<> m_imGUIRenderReceiver;

		bool m_oldCameraFound = false;
		bool m_shouldUpdateRenderPipeline = true;
	};
}