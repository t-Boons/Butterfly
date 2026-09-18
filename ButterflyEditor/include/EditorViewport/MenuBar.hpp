#pragma once
#include "Butterfly.hpp"
#include "EditorViewport/EditorViewportExtention.hpp"

namespace Butterfly
{
	class MenuBar : public IEditorViewportExtention
	{
	public:
		MenuBar();
		~MenuBar();

		virtual void OnTick() override;
		virtual void OnRenderImGUI() override;
	};
}