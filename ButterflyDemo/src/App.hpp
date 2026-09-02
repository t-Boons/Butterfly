#pragma once
#include "Butterfly.hpp"
#include "Input/Input.hpp"
#include "Tools/ImGUIRenderer.hpp"
#include <string_view>
#include "Tools/SpectatorCamera.hpp"

namespace Butterfly
{
	class SandboxLayer : public Butterfly::ApplicationLayer
	{
	public:
		virtual void OnInit();
		virtual void OnTick();
		virtual void OnShutdown();
		void Render();
		void ImGuiRender(const Graph* graph);
		void OnResize(const Butterfly::WindowResizeEvent& ev);
		void SetCompositeBufferResolutionIfChanged(uint32_t width, uint32_t height);

		inline static Window* StaticWindow;
		inline static BFTexture* CompositeTexture = nullptr;
	private:
		Input m_input;
		ScopePtr<Window> m_window;
		RefPtr<D3D12CommandList> m_cmdList;

		ImGUIRenderer m_imGUi;
		glm::vec2 m_oldWindowSize;

		BFTexture* m_screenTexture;

		bool m_imguiEnabled = true;


		ScopePtr<GraphTransientResourceCache> m_graphResources;
		ScopePtr<Blackboard> m_blackBoard;

		ScopePtr<BFIndexBuffer> m_modelIndices;
		ScopePtr<BFStructuredBuffer> m_modelPositions;
		ScopePtr<BFStructuredBuffer> m_modelNormals;
		ScopePtr<BFStructuredBuffer> m_modelUVS;
		ScopePtr<BFTexture> m_modelAlbedo;

		ScopePtr<BFUniformBuffer> m_uniforms;

		
		const std::string_view m_demos[2] = { "None", "ForwardSponza"};
		std::string m_currentDemo;
		SpectatorCamera m_spectatorCam;
	};
}