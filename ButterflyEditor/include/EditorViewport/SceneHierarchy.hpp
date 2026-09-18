#pragma once
#include "Butterfly.hpp"
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

	private:
		void DrawHierarchy(const TransformComponent& parent, uint32_t rowIndex, uint32_t columIndex);
		
		std::unordered_set<UUID> m_collapsedHeaderMap;
	};
}