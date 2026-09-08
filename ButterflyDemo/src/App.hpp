#pragma once
#include "Butterfly.hpp"
#include "Input/Input.hpp"
#include <string_view>
#include "Tools/SpectatorCamera.hpp"

namespace Butterfly
{
	class SandboxLayer : public Butterfly::Layer
	{
	public:
		virtual void OnInit();
		virtual void OnTick();
		virtual void OnShutdown();

		void ImGUIRender(D3D12CommandList& list);

	private:
		Input m_input;
		SpectatorCamera m_spectatorCam;

		float m_modelMovementTime = 0;
		Entity model;
	};
}