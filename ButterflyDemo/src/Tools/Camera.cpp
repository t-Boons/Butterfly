#include "Camera.hpp"

namespace Butterfly
{
	Camera::Camera(const CameraProjection& projection)
	{
		SetProjection(projection);
	}

	void Camera::SetProjection(const CameraProjection& projection)
	{
		m_projection = projection;
		m_projectionMatrix = glm::perspective(projection.Fov, projection.AspectRatio, projection.ZNear, projection.ZFar);
		UpdateMatrices();
	}

	void Camera::SetPosition(const glm::vec3& position)
	{
		m_position = position;
		UpdateMatrices();
	}

	void Camera::SetRotation(const glm::quat& rotation)
	{
		m_rotation = rotation;
		UpdateMatrices();
	}

	void Camera::UpdateMatrices()
	{
		m_viewMatrix = glm::inverse(glm::translate(glm::mat4(1.0f), m_position) * glm::toMat4(m_rotation));
		m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
	}
}