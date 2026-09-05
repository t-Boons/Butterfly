#pragma once
#include "Common.hpp"

namespace Butterfly
{
	class Application;
	class ApplicationLayer : public NonCopyable
	{
	public:
		virtual void OnInit() {}
		virtual void OnTick() {}
		virtual void OnShutdown() {}

	protected:
		friend class Application;
		Application* m_app = nullptr;
	};

	class Window;
	class Renderer;
	class Blackboard;

	class Application : public NonCopyable
	{
	public:
		void Init();
		void Update();
		void Quit();

		static Application& Get() { return *m_thisApp; }
		Window& GetWindow() { return *m_window; }
		Renderer& GetRenderer() { return *m_renderer; }
		Blackboard& GetBlackboard() { return *m_blackboard; }

		template<typename T>
		void AttachLayer()
		{
			m_layers.push_back(MakeRef<T>());
			m_layers.back()->m_app = this;
		}

	private:
		inline static Application* m_thisApp;
		bool m_running = true;
		std::vector<RefPtr<ApplicationLayer>> m_layers;

		Window* m_window;
		Renderer* m_renderer;
		Blackboard* m_blackboard;
	};
}