#pragma once

#include "antwika/gfx/Math3D.hpp"

namespace antwika::gfx
{

    struct Orthographic final
    {
        float halfWidth = 1.0F;

        float halfHeight = 1.0F;

        float offsetX = 0.0F;

        float offsetY = 0.0F;

        float nearPlane = -100.0F;

        float farPlane = 100.0F;

        [[nodiscard]] bool operator==(
            const Orthographic &other) const = default;
    };

}
