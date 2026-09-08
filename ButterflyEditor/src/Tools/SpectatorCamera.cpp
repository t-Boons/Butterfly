#include "SpectatorCamera.hpp"

namespace Butterfly
{
	void SpectatorCamera::Tick(const Input& input, float deltaTime)
	{
		if (!input.IsMousePressed(BFB_BTN_RIGHT))
			return;

		// Control camera position.
		glm::vec3 movementDelta = { 0,0,0 };
		movementDelta += m_camera->Right() * (input.IsKeyPressed(BFB_A) ? -1 : 0 + input.IsKeyPressed(BFB_D) ? 1.0f : 0);
		movementDelta += m_camera->Forward() * (input.IsKeyPressed(BFB_S) ? -1 : 0 + input.IsKeyPressed(BFB_W) ? 1.0f : 0);
		movementDelta += m_camera->Up() * (input.IsKeyPressed(BFB_Q) ? -1 : 0 + input.IsKeyPressed(BFB_E) ? 1.0f : 0);

		m_camera->SetPosition(m_camera->Position() + movementDelta * deltaTime * m_speed);

		// Control camera rotation;
		const float sensivitiy = m_sensitivity;
		m_MouseDelta.x += input.MouseXDelta() * sensivitiy;
		m_MouseDelta.y += input.MouseYDelta() * sensivitiy;
		m_camera->SetRotation(glm::quat({ 0, -m_MouseDelta.x, 0 }) * glm::quat({ -m_MouseDelta.y, 0, 0 }));


		m_speed += input.Scroll();
		if (m_speed < 0) m_speed = 0;
	}
}