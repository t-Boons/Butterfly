#include "Scene/Registry/TransformComponent.hpp"
#include "Scene/Entity.hpp"

namespace Butterfly
{
	void TransformComponent::SetPosition(const glm::vec3& position)
	{
		m_position = position;
		InvalidateMatrix();
	}

	void TransformComponent::SetRotation(const glm::quat& rotation)
	{
		m_rotation = rotation;
		InvalidateMatrix();
	}

	void TransformComponent::SetScale(const glm::vec3& scale)
	{
		m_scale = scale;
		InvalidateMatrix();
	}

	void TransformComponent::SetChild(Entity entity)
	{
		m_child = entity;
	}

	const glm::mat4& TransformComponent::GetMatrix()
	{
		if (m_isMatrixDirty)
		{
			m_matrix = glm::translate(glm::mat4(1.0f), m_position) *
				glm::mat4_cast(m_rotation) *
				glm::scale(glm::mat4(1.0f), m_scale);


			if (m_child)
			{
				TransformComponent& tr = m_child.GetComponent<TransformComponent>();
				m_matrix = tr.GetMatrix() * m_matrix;
			}	

			m_isMatrixDirty = false;
		}
		return m_matrix;
	}

	void TransformComponent::InvalidateMatrix()
	{
		m_isMatrixDirty = true;
	}
}