#include "Core/EditorApplication.hpp"
#include "EditorCache/EditorCache.hpp"
#include "EditorViewport/EditorViewport.hpp"
#include "Core/DebugRenderer.hpp"

namespace Butterfly
{
	void EditorApplication::OnInit()
	{
		BF_PROFILE_EVENT()

		s_instance = this;

		m_editorCache = new EditorCache();
		m_editorViewport = new EditorViewport();
		m_debugRenderer = new DebugRenderer();
	}

	void EditorApplication::OnTick()
	{
		BF_PROFILE_EVENT()

		m_editorViewport->Tick();
	}

	void EditorApplication::OnShutdown()
	{
		BF_PROFILE_EVENT()

		delete m_debugRenderer;
		delete m_editorCache;
		delete m_editorViewport;
	}
}
