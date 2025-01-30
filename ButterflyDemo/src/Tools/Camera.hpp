#pragma once
#include "glm/glm.hpp"
#include "glm/gtx/quaternion.hpp"

namespace Butterfly
{
	struct CameraProjection
	{
		float Fov = glm::degrees(45.0f);
		float AspectRatio = 16.0f / 9.0f;
		float ZNear = 0.01f;
		float ZFar = 100.0f;
	};

	class Camera
	{
	public:
		Camera(const CameraProjection& projection = CameraProjection());

		void SetProjection(const CameraProjection& projection);
		void SetPosition(const glm::vec3& position);
		void SetRotation(const glm::quat& rotation);

		const glm::vec3& Position() const { return m_position; }
		const glm::quat& Rotation() const { return m_rotation; }
		const glm::mat4& World() const { return m_viewMatrix; }
		CameraProjection Projection() const { return m_projection; }

		glm::vec3 Forward() const { return m_rotation * glm::vec3(0.0f, 0.0f, -1.0f); }
		glm::vec3 Right() const { return m_rotation * glm::vec3(1.0f, 0.0f, 0.0f); }
		glm::vec3 Up() const { return m_rotation * glm::vec3(0.0f, 1.0f, 0.0f); }

		const glm::mat4& ViewMatrix() const { return m_viewMatrix; }
		const glm::mat4& ProjectionMatrix() const { return m_projectionMatrix; }
		const glm::mat4& ViewProjectionMatrix() const { return m_viewProjectionMatrix; }

	private:
		void UpdateMatrices();

		CameraProjection m_projection;

		glm::mat4 m_projectionMatrix = glm::mat4(1.0f);
		glm::mat4 m_viewProjectionMatrix = glm::mat4(1.0f);
		glm::mat4 m_viewMatrix = glm::mat4(1.0f);
		glm::vec3 m_position = glm::vec3{ 0.0f };
		glm::quat m_rotation = glm::identity<glm::quat>();
	};
}