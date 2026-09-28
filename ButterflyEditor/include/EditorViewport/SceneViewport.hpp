#pragma once
#include "Butterfly.hpp"
#include "EditorViewport/SpectatorCamera.hpp"
#include "EditorViewport/EditorViewportExtention.hpp"

namespace Butterfly
{
	class SceneViewport : public IEditorViewportExtention
	{
	public:
		SceneViewport();
		~SceneViewport();

		virtual void OnTick() override;
		virtual void OnRenderImGUI() override;

		void OnResize(const ViewportResizeEvent& event);
		void OnPrerender(const ViewportPrerenderEvent& event);
		void RenderObjectPicker(const ViewportRenderEvent& event);

	private:
		ViewportHandle m_viewportHandle;
		SpectatorCamera m_spectatorCam;

		EventReceiver<ViewportResizeEvent> m_viewportResizeReceiver;
		EventReceiver<ViewportPrerenderEvent> m_viewportPrerenderReceiver;
	};
}