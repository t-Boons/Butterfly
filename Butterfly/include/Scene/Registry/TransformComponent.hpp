#pragma once
#include "Core/Common.hpp"
#include "Scene/Entity.hpp"
#include "Core/UUID.hpp"

namespace Butterfly
{		
	class TransformComponent
	{
	public:
		TransformComponent(const Entity& thisEntity);

		void SetPosition(const glm::vec3& position);
		void SetRotation(const glm::quat& rotation);
		void SetScale(const glm::vec3& scale);
		void SetWorldMatrix(const glm::mat4& matrix);
		void SetLocalMatrix(const glm::mat4& matrix);
		void Attach(TransformComponent& other, uint32_t childIndex = 0);

		const glm::vec3& GetPosition() const { return m_position; }
		const glm::quat& GetRotation() const { return m_rotation; }
		const glm::vec3& GetScale() const { return m_scale; }
		Entity GetChild(uint32_t index) const { return m_children[index]; }
		const glm::mat4& GetLocalMatrix() const { return m_localMatrix; }
		const glm::mat4& GetWorldMatrix();

		bool IsChildOf(const TransformComponent& other) const;

		bool HasChildren() const { return !m_children.empty(); }
		uint32_t NumChildren() const { return static_cast<uint32_t>(m_children.size()); }

		void AttachAndMoveAboveChild(const TransformComponent& child, TransformComponent& newChild);

		Entity GetParent() const { return m_parent; }
		Entity GetRoot() const;
	private:
		friend class Scene;
		friend class SceneManager;
		friend class ComponentRegistry;

		void DetachParent();

		void SetChildrenUUIDs(const std::vector<UUID>& uuids);
		std::vector<UUID> GetChildrenUUIDs() const;
		void SetParentUUID(const UUID& uuid) { m_parentUUID = uuid; }
		const UUID& GetParentUUID() const { return m_parentUUID; }

		void InvalidateMatrix();

		void ValidateAfterDeserialization(Scene& scene);

		glm::vec3 m_position = { 0.0f, 0.0f, 0.0f };
		glm::quat m_rotation = { 1.0f, 0.0f, 0.0f, 0.0f };
		glm::vec3 m_scale = { 1.0f, 1.0f, 1.0f };

		glm::mat4 m_matrix = glm::mat4(1.0f);
		glm::mat4 m_localMatrix = glm::mat4(1.0f);

		Entity m_thisEntity;

		Entity m_parent;
		UUID m_parentUUID;
		std::vector<Entity> m_children;
		std::vector<UUID> m_childrenUUIDs;

		bool m_isMatrixDirty = false;
	};
}