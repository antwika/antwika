#pragma once

#include <variant>

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

    struct Orthographic final
    {
        float halfWidth = 1.0F;

        float halfHeight = 1.0F;

        float nearPlane = -100.0F;

        float farPlane = 100.0F;

        [[nodiscard]] bool operator==(
            const Orthographic &other) const = default;
    };

    class Camera3D final
    {
    public:
        using Projection = std::variant<Perspective, Orthographic>;

        Camera3D() = default;

        Camera3D(
            Vec3 position,
            Vec3 target,
            Vec3 up,
            Projection projection);

        [[nodiscard]] Vec3 position() const;

        [[nodiscard]] Vec3 target() const;

        [[nodiscard]] Vec3 up() const;

        [[nodiscard]] const Projection &projection() const;

        void setPosition(Vec3 value);

        void setTarget(Vec3 value);

        void setProjection(Projection value);

        [[nodiscard]] Mat4 view() const;

        [[nodiscard]] Mat4 projectionMatrix() const;

        [[nodiscard]] Mat4 viewProjection() const;

    private:
        Vec3 eye{0.0F, 0.0F, 0.0F};
        Vec3 lookAt{0.0F, 0.0F, -1.0F};
        Vec3 upward{0.0F, 1.0F, 0.0F};
        Projection how{Perspective{}};
    };

}
