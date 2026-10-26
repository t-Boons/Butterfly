#pragma once
#include "Butterfly.hpp"

namespace Butterfly::Game
{
	class GameViewport
	{
	public:
		GameViewport();
		~GameViewport();

		void OnTick();
		void OnRenderImGUI();

		void OnPrerender(const ViewportPrerenderEvent& event);
		void RenderObjectPicker(const ViewportRenderEvent& event);

	private:
		ViewportHandle m_viewportHandle;
		uint32_t m_mainCameraIndex = 0;

		EventReceiver<ViewportResizeEvent> m_viewportResizeReceiver;
		EventReceiver<ViewportPrerenderEvent> m_viewportPrerenderReceiver;
		EventReceiver<> m_imGUIRenderReceiver;

		bool m_oldCameraFound = false;
		bool m_shouldUpdateRenderPipeline = true;
	};
}