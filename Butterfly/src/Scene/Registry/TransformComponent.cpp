#include "Scene/Registry/TransformComponent.hpp"
#include "Scene/Registry/IDComponent.hpp"
#include "Scene/Entity.hpp"
#include "Core/Application.hpp"
#include "Scene/Scene.hpp"

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

	void TransformComponent::Attach(TransformComponent& other, uint32_t childIndex)
	{
		BF_CORE_ASSERT(other.m_thisEntity.Valid(), "Entity is not valid for attachment");

		if (childIndex > m_children.size())
		{
			BF_CORE_LOG_WARN("Child index %i is out of bounds for entity %i", childIndex, m_thisEntity.GetHandle());
			return;
		}

		if (other.m_thisEntity == m_thisEntity)
		{
			BF_CORE_LOG_WARN("Unable to attach entity to itself: %i", m_thisEntity.GetHandle());
			return;
		}

		if (IsChildOf(other))
		{
			BF_CORE_LOG_WARN("Entity is already a child of this entity: %i", other.m_thisEntity.GetHandle());
			return;
		}

		auto it = std::find(m_children.begin(), m_children.end(), other.m_thisEntity);

		if (it != m_children.end())
		{
			const uint32_t removedIndex = static_cast<uint32_t>(std::distance(m_children.begin(), it));

			m_children.erase(it);

			if (removedIndex < childIndex)
			{
				--childIndex;
			}
		}
		else
		{
			other.DetachParent();
			other.m_parent = m_thisEntity;
		}

		m_children.insert(m_children.begin() + childIndex, other.m_thisEntity);

		other.InvalidateMatrix();
		InvalidateMatrix();
	}

	bool TransformComponent::IsChildOf(const TransformComponent& other) const
	{
		const TransformComponent* current = this;
		while (current->m_parent)
		{
			if (current->m_parent == other.m_thisEntity)
			{
				return true;
			}
			current = &current->m_parent.GetComponent<TransformComponent>();
		}
		return false;
	}

	const glm::mat4& TransformComponent::GetMatrix()
	{
		if (m_isMatrixDirty)
		{
			m_matrix = glm::translate(glm::mat4(1.0f), m_position) *
				glm::mat4_cast(m_rotation) *
				glm::scale(glm::mat4(1.0f), m_scale);


			if (m_parent)
			{
				m_matrix = m_parent.GetComponent<TransformComponent>().GetMatrix() * m_matrix;
			}	
			else
			{
				m_matrix = glm::mat4(1.0f);
			}

			m_isMatrixDirty = false;
		}
		return m_matrix;
	}

	void TransformComponent::AttachAndMoveAboveChild(const TransformComponent& child, TransformComponent& newChild)
	{
		Attach(newChild, std::distance(m_children.begin(), std::find(m_children.begin(), m_children.end(), child.m_thisEntity)));
	}

	void TransformComponent::SetChildrenUUIDs(const std::vector<UUID>& uuids)
	{
		m_childrenUUIDs = uuids;
	}
	std::vector<UUID> TransformComponent::GetChildrenUUIDs() const
	{
		return m_childrenUUIDs;
	}

	void TransformComponent::ValidateChildren()
	{

	}


	void TransformComponent::InvalidateMatrix()
	{
		m_isMatrixDirty = true;
		for (auto& child : m_children)
		{
			child.GetComponent<TransformComponent>().InvalidateMatrix();
		}
	}

	void TransformComponent::DetachParent()
	{
		if (m_parent)
		{
			TransformComponent& parentTransform = m_parent.GetComponent<TransformComponent>();
			parentTransform.m_children.erase(
				std::remove(parentTransform.m_children.begin(), parentTransform.m_children.end(), m_thisEntity),
				parentTransform.m_children.end());
		}
	}
}