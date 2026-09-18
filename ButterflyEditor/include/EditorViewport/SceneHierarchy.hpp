#pragma once
#include "Butterfly.hpp"
#include "EditorViewport/SpectatorCamera.hpp"
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

		void DrawHierarchy(const TransformComponent& parent, uint32_t rowIndex, uint32_t columIndex);

	private:
		std::unordered_map<UUID, bool> m_collapsedHeaderMap;
	};
}