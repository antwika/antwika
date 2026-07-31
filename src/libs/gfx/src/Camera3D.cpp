#include "antwika/gfx/Camera3D.hpp"

#include <variant>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "antwika/gfx/Math3D.hpp"

namespace antwika::gfx
{

    namespace
    {

        /**
         * @brief Turn one projection into its matrix.
         */
        struct ProjectionMatrix
        {
            [[nodiscard]] Mat4 operator()(
                const Perspective &value) const
            {
                return glm::perspective(
                    value.fovYRadians,
                    value.aspectRatio,
                    value.nearPlane,
                    value.farPlane);
            }

            [[nodiscard]] Mat4 operator()(
                const Orthographic &value) const
            {
                return glm::ortho(
                    -value.halfWidth,
                    value.halfWidth,
                    -value.halfHeight,
                    value.halfHeight,
                    value.nearPlane,
                    value.farPlane);
            }
        };

    } // namespace

    Camera3D::Camera3D(
        Vec3 position, Vec3 target, Vec3 up, Projection projection)
        : eye(position)
        , lookAt(target)
        , upward(up)
        , how(projection)
    {
    }

    Vec3 Camera3D::position() const
    {
        return eye;
    }

    Vec3 Camera3D::target() const
    {
        return lookAt;
    }

    Vec3 Camera3D::up() const
    {
        return upward;
    }

    const Camera3D::Projection &Camera3D::projection() const
    {
        return how;
    }

    void Camera3D::setPosition(Vec3 value)
    {
        eye = value;
    }

    void Camera3D::setTarget(Vec3 value)
    {
        lookAt = value;
    }

    void Camera3D::setProjection(Projection value)
    {
        how = value;
    }

    Mat4 Camera3D::view() const
    {
        // glm::lookAt normalises the eye-to-target direction, which is
        // a division by zero when the two coincide. Reporting the
        // identity keeps a drawing call producing numbers rather than
        // NaNs, and a camera sitting on its own target has nothing to
        // show anyway.
        if (eye == lookAt)
        {
            return identityMatrix();
        }

        return glm::lookAt(eye, lookAt, upward);
    }

    Mat4 Camera3D::projectionMatrix() const
    {
        return std::visit(ProjectionMatrix{}, how);
    }

    Mat4 Camera3D::viewProjection() const
    {
        return projectionMatrix() * view();
    }

} // namespace antwika::gfx
