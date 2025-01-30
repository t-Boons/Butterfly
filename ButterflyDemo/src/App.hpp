#pragma once
#include "Butterfly.hpp"
#include "Input/Input.hpp"
#include "GraphExamples/GraphExamples.hpp"
#include "Tools/ImGUIRenderer.hpp"
#include <string_view>
#include "Tools/SpectatorCamera.hpp"

namespace Butterfly
{
	class App
	{
	public:
		void Init();
		void Tick();
		void ShutDown();
		void ImGuiRender(const Graph* graph);
		void OnResize(const Butterfly::WindowResizeEvent& ev);

		bool ShouldShutDown = false;

		inline static Window* StaticWindow;
	private:
		Input m_input;
		ScopePtr<Window> m_window;
		RefPtr<DX12CommandList> m_cmdList;

		ImGUIRenderer m_imGUi;

		bool m_imguiEnabled = true;


		ScopePtr<GraphTransientResourceCache> m_graphResources;
		ScopePtr<Blackboard> m_blackBoard;


		ScopePtr<GraphExamples::GraphExample> m_graphExample;
		const std::string_view m_demos[3] = { "None", "DeferredSponza", "ForwardSponza"};
		std::string m_currentDemo;
		SpectatorCamera m_spectatorCam;
	};
}