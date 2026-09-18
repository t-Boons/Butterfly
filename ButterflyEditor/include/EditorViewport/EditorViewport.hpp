#pragma once
#include "Butterfly.hpp"

namespace Butterfly
{
	class IEditorViewportExtention;
	class EditorViewport
	{
	public:
		EditorViewport();

		void Tick();
		void OnRenderImGUI();

		Entity m_selectedEntity;
	private:
		float m_modelMovementTime = 0;

		std::vector<RefPtr<IEditorViewportExtention>> m_viewportExtentions;
		EventReceiver<> m_ImGUIRenderReceiver;
	};
}