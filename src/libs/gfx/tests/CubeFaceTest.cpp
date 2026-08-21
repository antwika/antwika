#include <gtest/gtest.h>

#include <cmath>
#include <set>
#include <vector>

#include "antwika/gfx/CubeFace.hpp"

namespace antwika::gfx
{

    namespace
    {
        constexpr float kTolerance = 0.0001F;

        [[nodiscard]] float lengthOf(const Vec3 vector)
        {
            return std::sqrt(
                (vector.x * vector.x) + (vector.y * vector.y)
                + (vector.z * vector.z));
        }

        [[nodiscard]] float dotOf(const Vec3 oneVector, const Vec3 otherVector)
        {
            return (oneVector.x * otherVector.x) + (oneVector.y * otherVector.y)
                   + (oneVector.z * otherVector.z);
        }

        TEST(CubeFaceTest, DirectionOf_LooksDownAnAxisAndNoOther)
        {
            for (const auto face : kEveryCubeFace)
            {
                const auto way = directionOf(face);

                EXPECT_NEAR(lengthOf(way), 1.0F, kTolerance);
                EXPECT_NEAR(
                    std::abs(way.x) + std::abs(way.y)
                        + std::abs(way.z),
                    1.0F,
                    kTolerance);
            }
        }

        TEST(CubeFaceTest, DirectionOf_LooksSixDifferentWays)
        {
            std::set<std::vector<int>> seenFaces;

            for (const auto face : kEveryCubeFace)
            {
                const auto way = directionOf(face);

                EXPECT_TRUE(
                    seenFaces.insert(
                            {static_cast<int>(way.x),
                             static_cast<int>(way.y),
                             static_cast<int>(way.z)})
                        .second);
            }

            EXPECT_EQ(seenFaces.size(), kCubeFaces);
        }

        TEST(CubeFaceTest, UpVectorOf_StandsSquareToTheWayItLooks)
        {
            for (const auto face : kEveryCubeFace)
            {
                const auto upVector = upVectorOf(face);

                EXPECT_NEAR(lengthOf(upVector), 1.0F, kTolerance);
                EXPECT_NEAR(dotOf(upVector, directionOf(face)), 0.0F,
                    kTolerance);
            }
        }
    }

}
