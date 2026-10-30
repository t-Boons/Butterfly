#pragma once
#include "Core/Common.hpp"
#include "Scene/Entity.hpp"

#define MAX_CASCADES 4
#define CUBEMAP_SLICES 6

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
		LightComponent(const Entity& entity);

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
			m_version++;	
		}

		void SetOuterConeAngle(float outerConeAngle)
		{
			m_outerConeAngle = outerConeAngle;
			m_version++;
		}

		void SetType(LightType type)
		{
			m_type = static_cast<uint32_t>(type);
			m_version++;

		}

		void SetColor(const glm::vec3& color)
		{
			m_color = color;
			m_version++;
		}

		void SetRange(float range)
		{
			m_range = range;
			m_version++;
		}

		bool CastsShadows() const
		{
			return m_castsShadows;
		}

		uint32_t GetNumCascades() const
		{
			return m_numCascades;
		}

		uint32_t GetShadowMapResolution() const
		{
			return m_shadowResolution;
		}

		std::array<float, MAX_CASCADES> GetCascadeSplits() const
		{
			return m_cascadeSplits;
		}

		uint32_t GetVersion() const
		{
			return m_version;
		}

	private:
		const uint32_t& GetTypeAsUInt() const
		{
			return m_type;
		}

		void SetTypeAsUInt(const uint32_t& type)
		{
			m_type = type;
			m_version++;
		}

		friend class ComponentRegistry;

		uint32_t m_type = static_cast<uint32_t>(LightType::Point);
		glm::vec3 m_color = { 1.0f, 1.0f, 1.0f };
		float m_range = 2.0f;
		float m_innerConeAngle = 30.0f;
		float m_outerConeAngle = 45.0f;

		bool m_castsShadows = true;

		std::array<float, MAX_CASCADES> m_cascadeSplits = { 8, 15, 25, 45 };
		uint32_t m_numCascades = 4;
		uint32_t m_shadowResolution = 1024;

		uint32_t m_version = 0;
	};
}