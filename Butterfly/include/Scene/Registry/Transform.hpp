#pragma once
#include "Core/Common.hpp"
#include "Scene/Entity.hpp"

namespace Butterfly
{
	class Entity;

	class Transform
	{
	public:
		void SetPosition(const glm::vec3& position);
		void SetRotation(const glm::quat& rotation);
		void SetScale(const glm::vec3& scale);
		void SetChild(Entity entity);

		const glm::vec3& GetPosition() const { return m_position; }
		const glm::quat& GetRotation() const { return m_rotation; }
		const glm::vec3& GetScale() const { return m_scale; }

		const glm::mat4& GetMatrix();


	private:
		void InvalidateMatrix();

		glm::vec3 m_position = { 0.0f, 0.0f, 0.0f };
		glm::quat m_rotation = { 1.0f, 0.0f, 0.0f, 0.0f };
		glm::vec3 m_scale = { 1.0f, 1.0f, 1.0f };

		glm::mat4 m_matrix = glm::mat4(1.0f);

		Entity m_child;

		bool m_isMatrixDirty = false;
	};
}