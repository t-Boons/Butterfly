#pragma once
#include "Common.hpp"

namespace Butterfly
{
	class Window;
	class Renderer;
	class Time;
	class Scene;
	class JobSystem;
	class AssetManager;
	class Input;

	class D3D12CommandList;
	class IApplicationExtention : public NonCopyableNonMoveable
	{
	public:
		virtual void OnInit() {}
		virtual void OnTick() {}
		virtual void OnShutdown() {}
		virtual void OnRender(D3D12CommandList& list) {}
	};

	class Application : public NonCopyableNonMoveable
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
		Input& GetInput() { return *m_input; }

		void SetApplicationExtention(IApplicationExtention* extention) { m_applicationExtention = extention; }

	private:
		inline static Application* s_instance;
		bool m_running = true;
		
		IApplicationExtention* m_applicationExtention;

		Window* m_window;
		Renderer* m_renderer;
		Time* m_time;
		Scene* m_scene;
		JobSystem* m_jobSystem;
		AssetManager* m_assetManager;
		Input* m_input;
	};
}