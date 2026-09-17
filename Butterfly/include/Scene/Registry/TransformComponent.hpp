#pragma once
#include "Core/Common.hpp"
#include "Scene/Entity.hpp"
#include "Core/UUID.hpp"

namespace Butterfly
{		
	class TransformComponent : public NonCopyableNonMoveable
	{
	public:
		void SetPosition(const glm::vec3& position);
		void SetRotation(const glm::quat& rotation);
		void SetScale(const glm::vec3& scale);
		void Attach(TransformComponent& other, uint32_t childIndex = 0);

		const glm::vec3& GetPosition() const { return m_position; }
		const glm::quat& GetRotation() const { return m_rotation; }
		const glm::vec3& GetScale() const { return m_scale; }
		const std::vector<Entity>& GetChildren() const { return m_children; }

		const glm::mat4& GetMatrix();

		bool IsChildOf(const TransformComponent& other) const;

		void AttachAndMoveAboveChild(const TransformComponent& child, TransformComponent& newChild);

		const Entity& GetParent() const { return m_parent; }
	private:
		friend class Scene;
		friend class ComponentRegistry;

		void DetachParent();

		void SetChildrenUUIDs(const std::vector<UUID>& uuids);
		std::vector<UUID> GetChildrenUUIDs() const;
		void ValidateChildren();


		void InvalidateMatrix();

		glm::vec3 m_position = { 0.0f, 0.0f, 0.0f };
		glm::quat m_rotation = { 1.0f, 0.0f, 0.0f, 0.0f };
		glm::vec3 m_scale = { 1.0f, 1.0f, 1.0f };

		glm::mat4 m_matrix = glm::mat4(1.0f);

		Entity m_thisEntity;
		Entity m_parent;

		std::vector<Entity> m_children;
		std::vector<UUID> m_childrenUUIDs;

		bool m_isMatrixDirty = false;
	};
}