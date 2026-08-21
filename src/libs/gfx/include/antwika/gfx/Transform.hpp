#pragma once

#include "antwika/gfx/Math3D.hpp"

namespace antwika::gfx
{

    struct Transform final
    {
        Vec3 position{0.0F, 0.0F, 0.0F};
        Vec3 rotationRadians{0.0F, 0.0F, 0.0F};
        Vec3 scale{1.0F, 1.0F, 1.0F};

        [[nodiscard]] Mat4 matrix() const;
    };

}
