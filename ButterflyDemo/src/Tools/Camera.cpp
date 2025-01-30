#include "Camera.hpp"

namespace Butterfly
{
	Camera::Camera(float fov, float aspectRatio, float zNear, float zFar)
		: m_fov(fov), m_aspectRatio(aspectRatio), m_near(zNear), m_far(zFar)
	{
		SetProjection(m_fov, m_aspectRatio, m_near, m_far);
	}

	void Camera::SetProjection(float fov, float aspectRatio, float zNear, float zFar)
	{
		m_projectionMatrix = glm::perspective(fov, aspectRatio, zNear, zFar);
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