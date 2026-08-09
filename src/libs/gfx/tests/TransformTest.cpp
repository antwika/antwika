#include <gtest/gtest.h>

#include <numbers>

#include "antwika/gfx/Math3D.hpp"
#include "antwika/gfx/Transform.hpp"

#include "MatrixApprox.hpp"

using antwika::gfx::identityMatrix;
using antwika::gfx::Transform;
using antwika::gfx::Vec3;
using antwika::gfx::Vec4;
using antwika::gfx::tests::approxEqual;

namespace
{
    constexpr float kQuarterTurn =
        std::numbers::pi_v<float> / 2.0F;

    Vec3 apply(const Transform &transform, Vec3 point)
    {
        const Vec4 result = transform.matrix() * Vec4(point, 1.0F);

        return Vec3(result);
    }
}

TEST(TransformTest, Matrix_DefaultTransformIsTheIdentity)
{
    EXPECT_TRUE(approxEqual(Transform{}.matrix(), identityMatrix()));
}

TEST(TransformTest, Matrix_TranslationMovesThePoint)
{
    const Transform transform{.translation = {1.0F, 2.0F, 3.0F}};

    EXPECT_TRUE(
        approxEqual(apply(transform, {0.0F, 0.0F, 0.0F}),
                    Vec3(1.0F, 2.0F, 3.0F)));
}

TEST(TransformTest, Matrix_ScaleMultipliesEachAxis)
{
    const Transform transform{.scale = {2.0F, 3.0F, 4.0F}};

    EXPECT_TRUE(
        approxEqual(apply(transform, {1.0F, 1.0F, 1.0F}),
                    Vec3(2.0F, 3.0F, 4.0F)));
}

TEST(TransformTest, Matrix_RotationAboutZTurnsXTowardsY)
{
    const Transform transform{
        .rotationRadians = {0.0F, 0.0F, kQuarterTurn}};

    EXPECT_TRUE(
        approxEqual(apply(transform, {1.0F, 0.0F, 0.0F}),
                    Vec3(0.0F, 1.0F, 0.0F)));
}

TEST(TransformTest, Matrix_RotationAboutYTurnsZTowardsX)
{
    const Transform transform{
        .rotationRadians = {0.0F, kQuarterTurn, 0.0F}};

    EXPECT_TRUE(
        approxEqual(apply(transform, {0.0F, 0.0F, 1.0F}),
                    Vec3(1.0F, 0.0F, 0.0F)));
}

TEST(TransformTest, Matrix_RotationAboutXTurnsYTowardsZ)
{
    const Transform transform{
        .rotationRadians = {kQuarterTurn, 0.0F, 0.0F}};

    EXPECT_TRUE(
        approxEqual(apply(transform, {0.0F, 1.0F, 0.0F}),
                    Vec3(0.0F, 0.0F, 1.0F)));
}

TEST(TransformTest, Matrix_ScalesThenRotatesThenTranslates)
{
    const Transform transform{
        .translation = {10.0F, 0.0F, 0.0F},
        .rotationRadians = {0.0F, 0.0F, kQuarterTurn},
        .scale = {2.0F, 1.0F, 1.0F}};

    EXPECT_TRUE(
        approxEqual(apply(transform, {1.0F, 0.0F, 0.0F}),
                    Vec3(10.0F, 2.0F, 0.0F)));
}
