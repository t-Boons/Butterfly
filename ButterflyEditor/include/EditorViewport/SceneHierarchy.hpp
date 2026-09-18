#pragma once
#include "Butterfly.hpp"
#include "EditorViewport/SpectatorCamera.hpp"
#include "EditorViewport/EditorViewportExtention.hpp"

namespace Butterfly
{
	class TransformComponent;

	class SceneHierarchy : public IEditorViewportExtention
	{
	public:
		SceneHierarchy();
		~SceneHierarchy();

		virtual void OnTick() override;
		virtual void OnRenderImGUI() override;

		void DrawHierarchy(const TransformComponent& parent, uint32_t rowIndex, uint32_t columIndex);

	private:
		RefPtr<BFTextureReadback> m_objectPickerReadback;
		ViewportHandle m_viewportHandle;
		SpectatorCamera m_spectatorCam;

		EventReceiver<ViewportResizeEvent> m_viewportResizeReceiver;
		EventReceiver<ViewportRenderEvent> m_viewportRenderReceiver;
		EventReceiver<ViewportPrerenderEvent> m_viewportPrerenderReceiver;
		EventReceiver<ViewportRenderEvent> m_skyboxRender;
	};
}