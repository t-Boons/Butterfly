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

		float GetConeAngle() const
		{
			return m_coneAngle;
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

		void SetConeAngle(float coneAngle)
		{
			m_coneAngle = coneAngle;
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

		uint32_t m_type;
		glm::vec3 m_color;
		float m_range;
		float m_coneAngle;
		bool m_dirty = true;
	};
}