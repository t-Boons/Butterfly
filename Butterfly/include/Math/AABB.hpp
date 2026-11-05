#pragma once
#include "Core/Common.hpp"

namespace Butterfly
{
	inline bool InRange(float min, float max, float value)
	{
		return value >= min && value <= max;
	}

    struct BFFrustum
    {
        glm::mat4 Projection;
        std::array<glm::vec4, 6> Planes;

		inline void UpdatePlanes()
		{
            Planes = std::array<glm::vec4, 6>
            {
                // Left
                glm::vec4{
                    Projection[0][3] + Projection[0][0],
                    Projection[1][3] + Projection[1][0],
                    Projection[2][3] + Projection[2][0],
                    Projection[3][3] + Projection[3][0]
                },
                    // Right
                glm::vec4{
                    Projection[0][3] - Projection[0][0],
                    Projection[1][3] - Projection[1][0],
                    Projection[2][3] - Projection[2][0],
                    Projection[3][3] - Projection[3][0]
                },
                    // Bottom
                glm::vec4{
                    Projection[0][3] + Projection[0][1],
                    Projection[1][3] + Projection[1][1],
                    Projection[2][3] + Projection[2][1],
                    Projection[3][3] + Projection[3][1]
                },
                    // Top
                glm::vec4{
                    Projection[0][3] - Projection[0][1],
                    Projection[1][3] - Projection[1][1],
                    Projection[2][3] - Projection[2][1],
                    Projection[3][3] - Projection[3][1]
                },
                    // Near
                glm::vec4{
                    Projection[0][3] + Projection[0][2],
                    Projection[1][3] + Projection[1][2],
                    Projection[2][3] + Projection[2][2],
                    Projection[3][3] + Projection[3][2]
                },
                    // Far
                glm::vec4{
                    Projection[0][3] - Projection[0][2],
                    Projection[1][3] - Projection[1][2],
                    Projection[2][3] - Projection[2][2],
                    Projection[3][3] - Projection[3][2]
                }
            };
		}
    };

    struct BFAABB
    {
		glm::vec3 Min = glm::vec3{ FLT_MAX };
        glm::vec3 Max = glm::vec3{ -FLT_MAX };

        inline glm::vec3 GetSize() const
		{
			return Max - Min;
		}

        inline bool IntersectsFrustum(const BFFrustum& frustum) const
        {
            for (const glm::vec4& plane : frustum.Planes)
            {
                const glm::vec3 normal = glm::vec3(plane);

                const glm::vec3 positive =
                {
                    normal.x >= 0.0f ? Max.x : Min.x,
                    normal.y >= 0.0f ? Max.y : Min.y,
                    normal.z >= 0.0f ? Max.z : Min.z
                };

                if (glm::dot(normal, positive) + plane.w < 0.0f)
                {
                    return false;
                }
            }

            return true;
        }

		inline bool IntersectAABB(const BFAABB& other) const
		{
			return (Min.x <= other.Max.x && Max.x >= other.Min.x) &&
				(Min.y <= other.Max.y && Max.y >= other.Min.y) &&
				(Min.z <= other.Max.z && Max.z >= other.Min.z);
		}

		inline bool ContainsPoint(const glm::vec3& point) const
		{
			return (point.x >= Min.x && point.x <= Max.x) &&
				(point.y >= Min.y && point.y <= Max.y) &&
				(point.z >= Min.z && point.z <= Max.z);
		}


		inline void ExpandToFit(const glm::vec3& point)
		{
			Min = glm::min(Min, point);
			Max = glm::max(Max, point);
		}

		inline std::array<glm::vec3, 8> GetCorners() const
		{
			return std::array<glm::vec3, 8>{
				glm::vec3{ Min.x, Min.y, Min.z },
				glm::vec3{ Max.x, Min.y, Min.z },
				glm::vec3{ Min.x, Max.y, Min.z },
				glm::vec3{ Max.x, Max.y, Min.z },
				glm::vec3{ Min.x, Min.y, Max.z },
				glm::vec3{ Max.x, Min.y, Max.z },
				glm::vec3{ Min.x, Max.y, Max.z },
				glm::vec3{ Max.x, Max.y, Max.z }
			};
		}

        inline BFAABB Transform(const glm::mat4& modelMatrix) const
        {
            BFAABB result{};
            const glm::vec3 center = (Min + Max) * 0.5f;
            const glm::vec3 extents = (Max - Min) * 0.5f;

            const glm::mat3 absMatrix(
                glm::abs(glm::vec3(modelMatrix[0])),
                glm::abs(glm::vec3(modelMatrix[1])),
                glm::abs(glm::vec3(modelMatrix[2]))
            );

            const glm::vec3 worldCenter = glm::vec3(modelMatrix * glm::vec4(center, 1.0f));
            const glm::vec3 worldExtents = absMatrix * extents;

            result.Min = worldCenter - worldExtents;
            result.Max = worldCenter + worldExtents;
            return result;
        }
    };
}