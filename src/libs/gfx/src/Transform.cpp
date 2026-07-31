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

    } // namespace

    Mat4 Transform::matrix() const
    {
        const Mat4 identity = identityMatrix();

        // Three stable-extension rotations, not glm::eulerAngleZYX.
        // That one is GTX, needing a global experimental define.
        const Mat4 rotation =
            glm::rotate(identity, rotationRadians.z, kZAxis)
            * glm::rotate(identity, rotationRadians.y, kYAxis)
            * glm::rotate(identity, rotationRadians.x, kXAxis);

        return glm::translate(identity, translation) * rotation
            * glm::scale(identity, scale);
    }

} // namespace antwika::gfx
