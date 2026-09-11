#include "EditorApplication.hpp"
#include "glm/gtx/quaternion.hpp"
#include <numeric>
#include "ImGUI/ImGUIHelpers.hpp"
#include "Core/ThumbnailProcessor.hpp"
#include "Core/EditorCache.hpp"
#include "Core/EditorViewport.hpp"

namespace Butterfly
{
	void EditorApplication::OnInit()
	{
		BF_PROFILE_EVENT()

		s_instance = this;

		m_editorCache = new EditorCache();
		m_editorViewport = new EditorViewport();
	}

	void EditorApplication::OnTick()
	{
		BF_PROFILE_FRAME("EditorApplication::OnTick");

		m_editorViewport->Tick();
	}

	void EditorApplication::OnShutdown()
	{
		BF_PROFILE_EVENT()

		delete m_editorCache;

		FullscreenQuad::ShutDown();
	}
}
