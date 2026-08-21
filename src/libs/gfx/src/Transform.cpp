#include "antwika/gfx/Transform.hpp"

#include <glm/ext/matrix_transform.hpp>

#include "antwika/gfx/Math3D.hpp"

namespace antwika::gfx
{

    namespace
    {

        constexpr Vec3 kXAxis{1.0F, 0.0F, 0.0F};
        constexpr Vec3 kYAxis{0.0F, 1.0F, 0.0F};
        constexpr Vec3 kZAxis{0.0F, 0.0F, 1.0F};

    }

    Mat4 Transform::matrix() const
    {
        const Mat4 identityTransform = identityMatrix();

        const Mat4 rotation =
            glm::rotate(identityTransform, rotationRadians.z, kZAxis)
            * glm::rotate(identityTransform, rotationRadians.y, kYAxis)
            * glm::rotate(identityTransform, rotationRadians.x, kXAxis);

        return glm::translate(identityTransform, position) * rotation
            * glm::scale(identityTransform, scale);
    }

}
