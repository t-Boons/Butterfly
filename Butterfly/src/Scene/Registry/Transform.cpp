#include "Scene/Registry/Transform.hpp"
#include "Scene/Entity.hpp"

namespace Butterfly
{
	void Transform::SetPosition(const glm::vec3& position)
	{
		m_position = position;
		InvalidateMatrix();
	}

	void Transform::SetRotation(const glm::quat& rotation)
	{
		m_rotation = rotation;
		InvalidateMatrix();
	}

	void Transform::SetScale(const glm::vec3& scale)
	{
		m_scale = scale;
		InvalidateMatrix();
	}

	void Transform::SetChild(Entity entity)
	{
		m_child = entity;
	}

	const glm::mat4& Transform::GetMatrix()
	{
		if (m_isMatrixDirty)
		{
			m_matrix = glm::translate(glm::mat4(1.0f), m_position) *
				glm::mat4_cast(m_rotation) *
				glm::scale(glm::mat4(1.0f), m_scale);


			if (m_child)
			{
				Transform& tr = m_child.GetComponent<Transform>();
				m_matrix = tr.GetMatrix() * m_matrix;
			}	

			m_isMatrixDirty = false;
		}
		return m_matrix;
	}

	void Transform::InvalidateMatrix()
	{
		m_isMatrixDirty = true;
	}
}