#pragma once
#include "Butterfly.hpp"

namespace Butterfly::Game
{
	class EditorCache;
	class EditorViewport;
	class DebugRenderer;
	class GameViewport;

	class GameApplication : public Butterfly::IApplicationExtention
	{
	public:
		virtual void OnInit();
		virtual void OnTick();
		virtual void OnShutdown();

		static GameApplication& Get() { return *s_instance; }

	private:
		inline static GameApplication* s_instance;

		GameViewport* m_viewport;
	};
}