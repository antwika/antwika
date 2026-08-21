#pragma once

#include "antwika/gfx/Math3D.hpp"

namespace antwika::gfx
{

    struct Perspective final
    {
        float fovYRadians = 1.0F;

        float aspectRatio = 1.0F;

        float nearPlane = 0.1F;

        float farPlane = 100.0F;

        [[nodiscard]] bool operator==(
            const Perspective &other) const = default;
    };

}
