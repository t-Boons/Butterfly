#pragma once
#include "Butterfly.hpp"
#include "EditorViewport/EditorViewportExtention.hpp"

namespace Butterfly
{
	class AssetLibrary : public IEditorViewportExtention
	{
	public:
		AssetLibrary();
		~AssetLibrary();

		virtual void OnTick() override;
		virtual void OnRenderImGUI() override;
	};
}