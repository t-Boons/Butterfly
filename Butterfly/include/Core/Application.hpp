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
		Application* m_app;
	};

	class Application : public NonCopyable
	{
	public:
		void Start();
		void Update();
		void Quit();

		template<typename T>
		void AttachLayer()
		{
			m_layers.push_back(MakeRef<T>());
			m_layers.back()->m_app = this;
		}

		template<typename T>
		void RemoveLayer()
		{
			m_layers.erase(std::remove_if(m_layers.begin(), m_layers.end(), [](const RefPtr<ApplicationLayer>& layer)
				{
					return dynamic_cast<T*>(layer.get()) != nullptr;
				}), m_layers.end());
		}

	private:
		bool m_running = true;
		std::vector<RefPtr<ApplicationLayer>> m_layers;
	};
}