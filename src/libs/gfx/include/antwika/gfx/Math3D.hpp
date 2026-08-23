#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace antwika::gfx
{

    using Vec2 = glm::vec2;

    using Vec3 = glm::vec3;

    using Vec4 = glm::vec4;

    using Mat4 = glm::mat4;

    [[nodiscard]] inline Mat4 getIdentityMatrix()
    {
        return Mat4(1.0F);
    }

}
