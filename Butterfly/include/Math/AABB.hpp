#pragma once
#include "Core/Common.hpp"

namespace Butterfly
{
	inline bool InRange(float min, float max, float value)
	{
		return value >= min && value <= max;
	}

    struct BFAABB
    {
		glm::vec3 Min = glm::vec3{ FLT_MAX };
        glm::vec3 Max = glm::vec3{ -FLT_MAX };

        inline glm::vec3 GetSize() const
		{
			return Max - Min;
		}

        inline bool IntersectsFrustum(const glm::mat4& mvp) const
        {
            const glm::vec4 planes[6] = {
                // Left
                {
                    mvp[0][3] + mvp[0][0],
                    mvp[1][3] + mvp[1][0],
                    mvp[2][3] + mvp[2][0],
                    mvp[3][3] + mvp[3][0]
                },
                // Right
                {
                    mvp[0][3] - mvp[0][0],
                    mvp[1][3] - mvp[1][0],
                    mvp[2][3] - mvp[2][0],
                    mvp[3][3] - mvp[3][0]
                },
                // Bottom
                {
                    mvp[0][3] + mvp[0][1],
                    mvp[1][3] + mvp[1][1],
                    mvp[2][3] + mvp[2][1],
                    mvp[3][3] + mvp[3][1]
                },
                // Top
                {
                    mvp[0][3] - mvp[0][1],
                    mvp[1][3] - mvp[1][1],
                    mvp[2][3] - mvp[2][1],
                    mvp[3][3] - mvp[3][1]
                },
                // Near
                {
                    mvp[0][3] + mvp[0][2],
                    mvp[1][3] + mvp[1][2],
                    mvp[2][3] + mvp[2][2],
                    mvp[3][3] + mvp[3][2]
                },
                // Far
                {
                    mvp[0][3] - mvp[0][2],
                    mvp[1][3] - mvp[1][2],
                    mvp[2][3] - mvp[2][2],
                    mvp[3][3] - mvp[3][2]
                }
            };

            for (const glm::vec4& plane : planes)
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
    };
}