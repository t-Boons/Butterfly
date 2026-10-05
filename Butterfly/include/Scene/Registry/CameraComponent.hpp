#pragma once
#include "Core/Common.hpp"
#include "Scene/Entity.hpp"

namespace Butterfly
{
	enum class CameraProjectionType
	{
		Perspective = 0,
		Orthographic = 1
	};

	class CameraComponent
	{
	public:
		CameraComponent(const Entity& entity)
		{
			ValidateProjectionMatrix();
		}

		void SetProjectionType(CameraProjectionType type)
		{
			m_projectionType = static_cast<uint32_t>(type);
			ValidateProjectionMatrix();
		}

		void SetSize(float size)
		{
			m_size = size;
			ValidateProjectionMatrix();
		}

		void SetFov(float fovDegrees)
		{
			m_fov = glm::radians(fovDegrees);
			ValidateProjectionMatrix();
		}

		void SetZNear(float zNear)
		{
			m_zNear = zNear;
			ValidateProjectionMatrix();
		}

		void SetZFar(float zFar)
		{
			m_zFar = zFar;
			ValidateProjectionMatrix();
		}

		void SetCameraIndex(uint32_t index)
		{
			m_cameraIndex = index;
		}

		void SetAspectRatio(float aspectRatio)
		{
			m_aspectRatio = aspectRatio;
			ValidateProjectionMatrix();
		}

		glm::mat4 GetViewprojectionMatrrix(const glm::vec3& position, const glm::quat& rotation) const
		{
			glm::mat4 view = GetViewMatrix(position, rotation);
			return m_projectionMatrix * view;
		}

		glm::mat4 GetViewMatrix(const glm::vec3& position, const glm::quat& rotation) const
		{
			return glm::mat4_cast(glm::inverse(rotation)) * glm::translate(glm::mat4(1.0f), -position);
		}

		CameraProjectionType GetProjectionType() const { return static_cast<CameraProjectionType>(m_projectionType); }
		float GetSize() const { return m_size; }
		float GetFov() const { return glm::degrees(m_fov); }
		float GetZNear() const { return m_zNear; }
		float GetZFar() const { return m_zFar;}
		const glm::mat4& GetProjectionMatrix() const { return m_projectionMatrix; }
		uint32_t GetCameraIndex() const { return m_cameraIndex; }
		float GetAspectRatio() const { return m_aspectRatio; }

	private:
		friend class ComponentRegistry;

		void SetProjectionTypeAsUInt(const uint32_t& type)
		{
			m_projectionType = type;
			ValidateProjectionMatrix();
		}


		const uint32_t& GetProjectionTypeAsUInt() const
		{
			return m_projectionType;
		}

		void ValidateProjectionMatrix()
		{
			if (m_projectionType == static_cast<uint32_t>(CameraProjectionType::Perspective))
			{
				m_projectionMatrix = glm::perspective(m_fov, m_aspectRatio, m_zNear, m_zFar);
			}
			else if (m_projectionType == static_cast<uint32_t>(CameraProjectionType::Orthographic))
			{
				float orthoHeight = m_size;
				float orthoWidth = orthoHeight * m_aspectRatio;
				m_projectionMatrix = glm::ortho(-orthoWidth / 2.0f, orthoWidth / 2.0f, -orthoHeight / 2.0f, orthoHeight / 2.0f, m_zNear, m_zFar);
			}
			else
			{
				BF_CORE_ASSERT(false, "Unknown projection type");
			}
		}


		uint32_t m_projectionType = static_cast<uint32_t>(CameraProjectionType::Perspective);
		float m_fov = glm::radians(90.0f);
		float m_aspectRatio = 1.0f;
		float m_zNear = 0.01f;
		float m_zFar = 100.0f;

		float m_size = 5.0f;

		uint32_t m_cameraIndex = 0;

		glm::mat4 m_projectionMatrix = glm::mat4(1.0f);
	};
}