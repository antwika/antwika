#include "antwika/gfx/Camera3D.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <variant>

#include "antwika/gfx/Math3D.hpp"

namespace antwika::gfx
{

    namespace
    {

        struct ProjectionMatrix final
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
                    value.offsetX - value.halfWidth,
                    value.offsetX + value.halfWidth,
                    value.offsetY - value.halfHeight,
                    value.offsetY + value.halfHeight,
                    value.nearPlane,
                    value.farPlane);
            }
        };

    }

    Camera3D::Camera3D(
        Vec3 position, Vec3 targetPoint, Vec3 upVector, Projection projection)
        : positionValue(position)
        , targetPoint(targetPoint)
        , upVector(upVector)
        , projectionValue(projection)
    {
    }

    Vec3 Camera3D::getPosition() const
    {
        return positionValue;
    }

    Vec3 Camera3D::getTarget() const
    {
        return targetPoint;
    }

    Vec3 Camera3D::getUp() const
    {
        return upVector;
    }

    const Camera3D::Projection &Camera3D::getProjection() const
    {
        return projectionValue;
    }

    void Camera3D::setPosition(Vec3 position)
    {
        positionValue = position;
    }

    void Camera3D::setTarget(Vec3 targetPosition)
    {
        targetPoint = targetPosition;
    }

    void Camera3D::setProjection(Projection valueProjection)
    {
        projectionValue = valueProjection;
    }

    Mat4 Camera3D::getView() const
    {
        if (positionValue == targetPoint)
        {
            return getIdentityMatrix();
        }

        return glm::lookAt(positionValue, targetPoint, upVector);
    }

    Mat4 Camera3D::getProjectionMatrix() const
    {
        return std::visit(ProjectionMatrix{}, projectionValue);
    }

    Mat4 Camera3D::getViewProjection() const
    {
        return getProjectionMatrix() * getView();
    }

}
