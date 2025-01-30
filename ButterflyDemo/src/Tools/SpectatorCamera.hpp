#pragma once
#include "Butterfly.hpp"
#include "Camera.hpp"
#include "../Input/Input.hpp"
#include <glm/glm.hpp>

namespace Butterfly
{
	class SpectatorCamera
	{
	public:
		SpectatorCamera(float speed = 15.0f, float sensitivity = 1.25f, RefPtr<Camera> camera = std::make_shared<Camera>())
			: m_camera(camera), m_speed(speed), m_sensitivity(sensitivity), m_MouseDelta({ 0,0 })
		{}

		void Tick(const Input& input, float deltaTime);

		const RefPtr<Camera>& GetCamera() const { return m_camera; }

	private:
		RefPtr<Camera> m_camera;
		glm::vec2 m_MouseDelta;
		float m_speed;
		float m_sensitivity;
	};
}