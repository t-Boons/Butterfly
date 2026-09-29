#pragma once
#include "Core/Common.hpp"
#include "Scene/Entity.hpp"

namespace Butterfly
{
	enum class LightType : uint32_t
	{
		Directional,
		Point,
		Spot
	};

	class LightComponent
	{
	public:
		LightComponent(const Entity& entity)
		{

		}

		LightType GetType() const
		{
			return static_cast<LightType>(m_type);
		}

		const glm::vec3& GetColor() const
		{
			return m_color;
		}

		float GetRange() const
		{
			return m_range;
		}

		float GetInnerConeAngle() const
		{
			return m_innerConeAngle;
		}

		float GetOuterConeAngle() const
		{
			return m_outerConeAngle;
		}

		void SetInnerConeAngle(float innerConeAngle)
		{
			m_innerConeAngle = innerConeAngle;
			m_dirty = true;
		}

		void SetOuterConeAngle(float outerConeAngle)
		{
			m_outerConeAngle = outerConeAngle;
			m_dirty = true;
		}

		void SetType(LightType type)
		{
			m_type = static_cast<uint32_t>(type);
			m_dirty = true;

		}

		void SetColor(const glm::vec3& color)
		{
			m_color = color;
			m_dirty = true;

		}

		void SetRange(float range)
		{
			m_range = range;
			m_dirty = true;
		}

		void ClearDirty()
		{
			m_dirty = false;
		}

	private:
		const uint32_t& GetTypeAsUInt() const
		{
			return m_type;
		}

		void SetTypeAsUInt(const uint32_t& type)
		{
			m_type = type;
		}

		friend class ComponentRegistry;

		uint32_t m_type = static_cast<uint32_t>(LightType::Point);
		glm::vec3 m_color = { 1.0f, 1.0f, 1.0f };
		float m_range = 2.0f;
		float m_innerConeAngle = 30.0f;
		float m_outerConeAngle = 45.0f;
		bool m_dirty = true;
	};
}