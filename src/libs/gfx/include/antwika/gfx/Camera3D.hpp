#pragma once

#include <variant>

#include "antwika/gfx/Math3D.hpp"

namespace antwika::gfx
{

    /**
     * @brief A projection that makes distant things smaller.
     */
    struct Perspective
    {
        /// Vertical field of view, in radians.
        float fovYRadians = 1.0F;

        /// Width divided by height of the area being drawn into.
        float aspectRatio = 1.0F;

        /// Distance to the near clip plane; must be above zero.
        float nearPlane = 0.1F;

        /// Distance to the far clip plane; must exceed nearPlane.
        float farPlane = 100.0F;

        /**
         * @brief Compare two perspectives field by field.
         * @param other The projection to compare against.
         * @return True when every field matches exactly.
         */
        [[nodiscard]] bool operator==(
            const Perspective &other) const = default;
    };

    /**
     * @brief A projection that keeps parallel lines parallel.
     *
     * Described by half its extents rather than by four planes, since
     * every use so far is centred on the camera and the asymmetric case
     * would be four numbers nothing sets differently.
     */
    struct Orthographic
    {
        /// Half the width of the visible volume, in world units.
        float halfWidth = 1.0F;

        /// Half its height, in world units.
        float halfHeight = 1.0F;

        /// Distance to the near clip plane; may be negative.
        float nearPlane = -100.0F;

        /// Distance to the far clip plane; must exceed nearPlane.
        float farPlane = 100.0F;

        /**
         * @brief Compare two orthographic projections field by field.
         * @param other The projection to compare against.
         * @return True when every field matches exactly.
         */
        [[nodiscard]] bool operator==(
            const Orthographic &other) const = default;
    };

    /**
     * @brief Where a scene is looked at from, and how it is flattened.
     *
     * Render-side only, like everything in Math3D.hpp.
     * A camera here is emphatically *not* what apps/game calls its
     * camera: that one is simulation state, because a click's meaning
     * depends on it, and it is integral for exactly that reason.
     * This one only ever produces matrices a drawing call consumes, so
     * nothing it computes can reach a replay.
     *
     * It holds no aspect ratio of its own -- that lives in Perspective,
     * where the projection that needs it is -- so resizing a window
     * changes the projection and leaves the eye where it was.
     */
    class Camera3D
    {
    public:
        /// Either kind of projection, chosen per camera.
        using Projection = std::variant<Perspective, Orthographic>;

        /**
         * @brief Construct a camera at the origin looking down -Z.
         */
        Camera3D() = default;

        /**
         * @brief Construct a camera looking at a point.
         * @param position Where the eye is, in world space.
         * @param target The point the eye is aimed at.
         * @param up Which way is up for the eye; need not be
         * perpendicular to the view direction, and need not be unit
         * length.
         * @param projection How the scene is flattened.
         */
        Camera3D(
            Vec3 position,
            Vec3 target,
            Vec3 up,
            Projection projection);

        /**
         * @brief Get where the eye is.
         * @return The position it was constructed or set with.
         */
        [[nodiscard]] Vec3 position() const;

        /**
         * @brief Get what the eye is aimed at.
         * @return The target it was constructed or set with.
         */
        [[nodiscard]] Vec3 target() const;

        /**
         * @brief Get which way is up for the eye.
         * @return The up vector it was constructed or set with.
         */
        [[nodiscard]] Vec3 up() const;

        /**
         * @brief Get how the scene is flattened.
         * @return The projection it was constructed or set with.
         */
        [[nodiscard]] const Projection &projection() const;

        /**
         * @brief Move the eye.
         * @param value Where to put it.
         */
        void setPosition(Vec3 value);

        /**
         * @brief Aim the eye.
         * @param value The point to look at.
         */
        void setTarget(Vec3 value);

        /**
         * @brief Choose how the scene is flattened.
         * @param value The projection to use.
         */
        void setProjection(Projection value);

        /**
         * @brief Build the matrix taking world space to eye space.
         * @return A right-handed look-at matrix; the identity is
         * returned when the eye sits exactly on its target, since there
         * is no direction to look in and a normalise would divide by
         * zero.
         */
        [[nodiscard]] Mat4 view() const;

        /**
         * @brief Build the matrix taking eye space to clip space.
         * @return The matrix the held projection describes.
         */
        [[nodiscard]] Mat4 projectionMatrix() const;

        /**
         * @brief Build the two together.
         * @return projectionMatrix() * view(), which is what a drawing
         * call multiplies a model matrix into.
         */
        [[nodiscard]] Mat4 viewProjection() const;

    private:
        Vec3 eye{0.0F, 0.0F, 0.0F};
        Vec3 lookAt{0.0F, 0.0F, -1.0F};
        Vec3 upward{0.0F, 1.0F, 0.0F};
        Projection how{Perspective{}};
    };

} // namespace antwika::gfx
