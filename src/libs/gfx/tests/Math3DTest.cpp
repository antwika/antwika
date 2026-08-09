#include <gtest/gtest.h>

#include "antwika/gfx/Math3D.hpp"

#include "MatrixApprox.hpp"

using antwika::gfx::identityMatrix;
using antwika::gfx::Vec4;
using antwika::gfx::tests::approxEqual;

TEST(Math3DTest, IdentityMatrix_LeavesAVectorAlone)
{
    const Vec4 point{1.0F, 2.0F, 3.0F, 1.0F};

    EXPECT_TRUE(approxEqual(identityMatrix() * point, point));
}
