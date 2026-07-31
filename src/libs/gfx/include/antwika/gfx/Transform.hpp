#pragma once

#include "antwika/gfx/Math3D.hpp"

namespace antwika::gfx
{

    /**
     * @brief Where a mesh sits, how it is turned and how big it is.
     *
     * Render-side only, like everything in Math3D.hpp: this is how a
     * drawing call is aimed, never how a simulation remembers a
     * position.
     *
     * Rotation is three Euler angles in radians, composed as Rz * Ry *
     * Rx, so a vertex is turned about X first and about Z last.
     * Euler angles rather than a quaternion because nothing here
     * interpolates between two orientations yet, and three named angles
     * read better in a test than four components do.
     */
    struct Transform
    {
        Vec3 translation{0.0F, 0.0F, 0.0F};
        Vec3 rotationRadians{0.0F, 0.0F, 0.0F};
        Vec3 scale{1.0F, 1.0F, 1.0F};

        /**
         * @brief Build the model matrix this transform describes.
         * @return Translation * rotation * scale, so a vertex is
         * scaled, then turned about the origin, then moved.
         */
        [[nodiscard]] Mat4 matrix() const;
    };

} // namespace antwika::gfx
