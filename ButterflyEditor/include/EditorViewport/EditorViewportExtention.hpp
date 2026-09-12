#pragma once

namespace Butterfly
{
	class IEditorViewportExtention
	{
	public:
		virtual void OnTick() = 0;
		virtual void OnRenderImGUI() = 0;
	};
}