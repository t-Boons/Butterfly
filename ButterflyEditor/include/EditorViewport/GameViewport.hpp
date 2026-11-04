#pragma once
#include "Butterfly.hpp"
#include "EditorViewport/SpectatorCamera.hpp"
#include "EditorViewport/EditorViewportExtention.hpp"

namespace Butterfly
{
	class GameViewport : public IEditorViewportExtention
	{
	public:
		GameViewport();

		virtual void OnTick() override;
		virtual void OnRenderImGUI() override;

		void OnPrerender(const ViewportPrerenderEvent& event);
		void RenderObjectPicker(const ViewportRenderEvent& event);

	private:
		RefPtr<Viewport> m_viewport;
		uint32_t m_mainCameraIndex = 0;

		EventReceiver<ViewportResizeEvent> m_viewportResizeReceiver;
		EventReceiver<ViewportPrerenderEvent> m_viewportPrerenderReceiver;
		bool m_oldCameraFound = false;
		bool m_shouldUpdateRenderPipeline = true;
	};
}