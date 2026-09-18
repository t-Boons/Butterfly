#pragma once
#include "Butterfly.hpp"
#include "EditorViewport/EditorViewportExtention.hpp"

namespace Butterfly
{
	class EntityProperties : public IEditorViewportExtention
	{
	public:
		EntityProperties();
		~EntityProperties();

		virtual void OnTick() override;
		virtual void OnRenderImGUI() override;
	};
}