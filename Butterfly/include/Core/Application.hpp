#pragma once
#include "Common.hpp"
#include "Core/Layer.hpp"

namespace Butterfly
{
	class Window;
	class Renderer;
	class Blackboard;
	class Time;
	class Scene;
	class JobSystem;
	class AssetManager;

	class Application : public NonCopyable
	{
	public:
		void Init();
		void Tick();
		void Quit();

		static Application& Get() { return *s_instance; }
		Window& GetWindow() { return *m_window; }
		Renderer& GetRenderer() { return *m_renderer; }
		Time& GetTime() { return *m_time; }
		Scene& GetScene() { return *m_scene; }
		JobSystem& GetJobSystem() { return *m_jobSystem; }
		AssetManager& GetAssetManager() { return *m_assetManager; }
		Blackboard& GetBlackboard() { return *m_blackboard; }

		template<typename T>
		void AttachLayer()
		{
			m_layers.push_back(MakeRef<T>());
		}

	private:
		inline static Application* s_instance;
		bool m_running = true;
		std::vector<RefPtr<Layer>> m_layers;

		Window* m_window;
		Renderer* m_renderer;
		Time* m_time;
		Scene* m_scene;
		JobSystem* m_jobSystem;
		AssetManager* m_assetManager;
		Blackboard* m_blackboard;
	};
}