#pragma once
#include "Butterfly.hpp"
#include "EditorViewport/SpectatorCamera.hpp"
#include "EditorViewport/EditorViewportExtention.hpp"
#include "ImGuizmo/ImGuizmo.h"

namespace Butterfly
{

	class SceneViewport : public IEditorViewportExtention
	{
	public:
		enum class ObjectMovementSpace
		{
			World,
			Local
		};

		SceneViewport();

		virtual void OnTick() override;
		virtual void OnRenderImGUI() override;

		void OnResize(const ViewportResizeEvent& event);
		void OnPrerender(const ViewportPrerenderEvent& event);
		void RenderObjectPicker(const ViewportRenderEvent& event);
		void OnPostRender();

	private:
		void RenderGrid();
		void RenderObjectHelpersImGui();
		void RenderObjectBoundingBoxes();


		RefPtr<Viewport> m_viewport;
		SpectatorCamera m_spectatorCam;

		EventReceiver<ViewportResizeEvent> m_viewportResizeReceiver;
		EventReceiver<ViewportPrerenderEvent> m_viewportPrerenderReceiver;
		EventReceiver<> m_viewportPostRenderReceiver;
		ObjectMovementSpace m_objectMovementSpace = ObjectMovementSpace::World;
		ImGuizmo::OPERATION m_currentOperation = ImGuizmo::OPERATION::TRANSLATE;
		uint32_t m_selectedViewType = 0;
		EventReceiver<Scene*> m_sceneChangedReceiver;

		bool m_boundingBoxesVisible = true;
		bool m_gridVisible = true;
		bool m_gizmosVisible = true;
	};
}