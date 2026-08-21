#pragma once

#include <variant>

#include "antwika/gfx/Math3D.hpp"
#include "antwika/gfx/Orthographic.hpp"
#include "antwika/gfx/Perspective.hpp"

namespace antwika::gfx
{

    class Camera3D final
    {
    public:
        using Projection = std::variant<Perspective, Orthographic>;

        Camera3D() = default;

        Camera3D(
            Vec3 position,
            Vec3 targetPoint,
            Vec3 upVector,
            Projection projection);

        [[nodiscard]] Vec3 position() const;

        [[nodiscard]] Vec3 target() const;

        [[nodiscard]] Vec3 up() const;

        [[nodiscard]] const Projection &projection() const;

        void setPosition(Vec3 position);

        void setTarget(Vec3 targetPosition);

        void setProjection(Projection valueProjection);

        [[nodiscard]] Mat4 view() const;

        [[nodiscard]] Mat4 projectionMatrix() const;

        [[nodiscard]] Mat4 viewProjection() const;

    private:
        Vec3 positionValue{0.0F, 0.0F, 0.0F};
        Vec3 targetPoint{0.0F, 0.0F, -1.0F};
        Vec3 upVector{0.0F, 1.0F, 0.0F};
        Projection projectionValue{Perspective{}};
    };

}
