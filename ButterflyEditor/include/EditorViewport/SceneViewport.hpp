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
		ObjectMovementSpace m_objectMovementSpace = ObjectMovementSpace::World;
		ImGuizmo::OPERATION m_currentOperation = ImGuizmo::OPERATION::TRANSLATE;
	};
}