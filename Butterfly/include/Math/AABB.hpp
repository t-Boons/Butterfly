#pragma once
#include "Core/Common.hpp"

namespace Butterfly
{
    struct BFAABB
    {
        glm::vec3 Size() const { return Max - Min; }
        glm::vec3 Min = glm::vec3{ 0.0 };
        glm::vec3 Max = glm::vec3{ 0.0 };
    };
}