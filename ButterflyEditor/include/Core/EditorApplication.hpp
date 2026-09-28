#pragma once
#include "Butterfly.hpp"

namespace Butterfly
{
	class EditorCache;
	class EditorViewport;
	class DebugRenderer;

	class EditorApplication : public Butterfly::IApplicationExtention
	{
	public:
		virtual void OnInit();
		virtual void OnTick();
		virtual void OnShutdown();

		static EditorApplication& Get() { return *s_instance; }
		EditorCache& GetEditorCache() { return *m_editorCache; }
		EditorViewport& GetEditorViewport() { return *m_editorViewport; }
		DebugRenderer& GetDebugRenderer() { return *m_debugRenderer; }

	private:
		inline static EditorApplication* s_instance;

		DebugRenderer* m_debugRenderer;
		EditorCache* m_editorCache;
		EditorViewport* m_editorViewport;
	};
}