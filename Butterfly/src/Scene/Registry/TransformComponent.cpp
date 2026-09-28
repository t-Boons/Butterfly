#include "Scene/Registry/TransformComponent.hpp"
#include "Scene/Registry/IDComponent.hpp"
#include "Scene/Entity.hpp"
#include "Core/Application.hpp"
#include "Scene/Scene.hpp"

namespace Butterfly
{
	TransformComponent::TransformComponent(const Entity& thisEntity)
		: m_thisEntity(thisEntity)
	{
	}

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

	void TransformComponent::SetLocalMatrix(const glm::mat4& matrix)
	{
		m_localMatrix = matrix;
		glm::vec3 scew;
		glm::vec4 perspective;
		glm::decompose(matrix, m_scale, m_rotation, m_position, scew, perspective);

		InvalidateMatrix();
	}
	void TransformComponent::SetWorldMatrix(const glm::mat4& matrix)
	{
		glm::mat4 parentMatrix = m_parent.GetComponent<TransformComponent>().GetWorldMatrix();

		SetLocalMatrix(glm::inverse(parentMatrix) * matrix);
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
			m_childrenUUIDs.erase(m_childrenUUIDs.begin() + removedIndex);

			if (removedIndex < childIndex)
			{
				--childIndex;
			}
		}
		else
		{
			other.DetachParent();
			other.m_parent = m_thisEntity;
			other.m_parentUUID = m_thisEntity.GetComponent<IDComponent>().EntityUUID;
		}

		m_children.insert(m_children.begin() + childIndex, other.m_thisEntity);
		m_childrenUUIDs.insert(m_childrenUUIDs.begin() + childIndex, other.m_thisEntity.GetComponent<IDComponent>().EntityUUID);

		other.InvalidateMatrix();
		InvalidateMatrix();
	}

	Entity TransformComponent::GetRoot() const
	{
		const TransformComponent* current = this;
		while (current->m_parent)
		{
			current = &current->m_parent.GetComponent<TransformComponent>();
		}
		return current->m_thisEntity;
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

	const glm::mat4& TransformComponent::GetWorldMatrix()
	{
		if (m_isMatrixDirty)
		{
			m_matrix = glm::translate(glm::mat4(1.0f), m_position) *
				glm::mat4_cast(m_rotation) *
				glm::scale(glm::mat4(1.0f), m_scale);

			m_localMatrix = m_matrix;

			if (m_parent)
			{
				m_matrix = m_parent.GetComponent<TransformComponent>().GetWorldMatrix() * m_matrix;
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
			for (uint32_t i = 0; i < parentTransform.m_children.size(); ++i)
			{
				if (parentTransform.m_children[i] == m_thisEntity)
				{
					parentTransform.m_children.erase(parentTransform.m_children.begin() + i);
					parentTransform.m_childrenUUIDs.erase(parentTransform.m_childrenUUIDs.begin() + i);
					break;
				}
			}
		}
	}

	void TransformComponent::ValidateAfterDeserialization(Scene& scene)
	{
		m_children.clear();
		for (const UUID& uuid : m_childrenUUIDs)
		{
			for (const auto& [entity, idComp] : scene.GetRegistry().view<IDComponent>().each())
			{
				if (idComp.EntityUUID == uuid)
				{
					m_children.push_back(Entity(entity));
					break;
				}
			}
		}

		for (const auto& [entity, idComp] : scene.GetRegistry().view<IDComponent>().each())
		{
			if (idComp.EntityUUID == m_parentUUID)
			{
				m_parent = Entity(entity);
				break;
			}
		}
	}
}