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
		void Render();
		void ShutDown();
		void ImGuiRender(const Graph* graph);
		void OnResize(const Butterfly::WindowResizeEvent& ev);
		void SetCompositeBufferResolutionIfChanged(uint32_t width, uint32_t height);

		bool ShouldShutDown = false;

		inline static Window* StaticWindow;
		inline static BFTexture* CompositeTexture = nullptr;
	private:
		Input m_input;
		ScopePtr<Window> m_window;
		RefPtr<DX12CommandList> m_cmdList;

		ImGUIRenderer m_imGUi;
		glm::vec2 m_oldWindowSize;

		BFTexture* m_screenTexture;

		bool m_imguiEnabled = true;


		ScopePtr<GraphTransientResourceCache> m_graphResources;
		ScopePtr<Blackboard> m_blackBoard;


		ScopePtr<GraphExamples::GraphExample> m_graphExample;
		const std::string_view m_demos[3] = { "None", "DeferredSponza", "ForwardSponza"};
		std::string m_currentDemo;
		SpectatorCamera m_spectatorCam;
	};
}