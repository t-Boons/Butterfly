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

		void RunBeforeImGuiRender(const std::function<void()>& func);

		Entity m_selectedEntity;
	private:

		std::vector<RefPtr<IEditorViewportExtention>> m_viewportExtentions;
		EventReceiver<> m_ImGUIRenderReceiver;
		std::queue<std::function<void()>> m_runBeforeImGuiRender;
	};
}