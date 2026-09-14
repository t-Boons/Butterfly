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
		entt::entity m_selectedEntity = entt::null;
		RefPtr<BFTextureReadback> m_objectPickerReadback;
		ViewportHandle m_viewportHandle;
		SpectatorCamera m_spectatorCam;

		EventHandle m_viewportResizeReceiver;
		EventHandle m_viewportRenderReceiver;
		EventHandle m_viewportPrerenderReceiver;

	};
}