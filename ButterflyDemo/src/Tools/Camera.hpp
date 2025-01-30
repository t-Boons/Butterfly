#pragma once
#include "glm/glm.hpp"
#include "glm/gtx/quaternion.hpp"

namespace Butterfly
{
	class Camera
	{
	public:
		Camera(float fov = 45 * 3.1415926f / 180.0f, float aspectRatio = 16.0f / 9.0f, float zNear = 0.01f, float zFar = 100.0f);

		void SetProjection(float fov, float aspectRatio, float zNear, float zFar);
		void SetPosition(const glm::vec3& position);
		void SetRotation(const glm::quat& rotation);

		const glm::vec3& Position() const { return m_position; }
		const glm::quat& Rotation() const { return m_rotation; }
		const glm::mat4& World() const { return m_viewMatrix; }

		glm::vec3 Forward() const { return m_rotation * glm::vec3(0.0f, 0.0f, -1.0f); }
		glm::vec3 Right() const { return m_rotation * glm::vec3(1.0f, 0.0f, 0.0f); }
		glm::vec3 Up() const { return m_rotation * glm::vec3(0.0f, 1.0f, 0.0f); }

		const glm::mat4& View() const { return m_viewMatrix; }
		const glm::mat4& Projection() const { return m_projectionMatrix; }
		const glm::mat4& ViewProjection() const { return m_viewProjectionMatrix; }

	private:
		void UpdateMatrices();

		float m_fov;
		float m_aspectRatio;
		float m_near;
		float m_far;

		glm::mat4 m_projectionMatrix = glm::mat4(1.0f);
		glm::mat4 m_viewProjectionMatrix = glm::mat4(1.0f);
		glm::mat4 m_viewMatrix = glm::mat4(1.0f);
		glm::vec3 m_position = glm::vec3{ 0.0f };
		glm::quat m_rotation = glm::identity<glm::quat>();
	};
}