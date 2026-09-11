#pragma once
#include "Butterfly.hpp"
#include "Input/Input.hpp"
#include <string_view>

namespace Butterfly
{
	class EditorCache;
	class EditorViewport;

	class EditorApplication : public Butterfly::IApplicationExtention
	{
	public:
		virtual void OnInit();
		virtual void OnTick();
		virtual void OnShutdown();

		static EditorApplication& Get() { return *s_instance; }
		EditorCache& GetEditorCache() { return *m_editorCache; }
		EditorViewport& GetEditorViewport() { return *m_editorViewport; }

	private:
		inline static EditorApplication* s_instance;

		EditorCache* m_editorCache;
		EditorViewport* m_editorViewport;
	};
}