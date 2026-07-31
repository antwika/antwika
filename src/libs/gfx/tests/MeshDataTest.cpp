#include <gtest/gtest.h>

#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/Math3D.hpp"
#include "antwika/gfx/MeshData.hpp"

using antwika::gfx::Color;
using antwika::gfx::MeshData;
using antwika::gfx::Vec2;
using antwika::gfx::Vec3;
using antwika::gfx::Vertex3D;

namespace
{
    MeshData oneTriangle()
    {
        return MeshData{
            .vertices =
                {Vertex3D{.position = Vec3{0.0F, 0.0F, 0.0F}},
                 Vertex3D{.position = Vec3{1.0F, 0.0F, 0.0F}},
                 Vertex3D{.position = Vec3{0.0F, 1.0F, 0.0F}}},
            .indices = {0U, 1U, 2U}};
    }
} // namespace

// A defaulted operator== stops at the first field that differs.
// So each one has to be asked about on its own.
// Otherwise the name of this test is not true of it.
TEST(Vertex3DTest, Equality_ComparesEveryField)
{
    const Vertex3D left;
    Vertex3D right;

    EXPECT_EQ(left, right);

    right = left;
    right.position = Vec3{1.0F, 0.0F, 0.0F};
    EXPECT_NE(left, right);

    right = left;
    right.normal = Vec3{0.0F, 1.0F, 0.0F};
    EXPECT_NE(left, right);

    right = left;
    right.texCoord = Vec2{1.0F, 0.0F};
    EXPECT_NE(left, right);

    right = left;
    right.color = Color{0, 0, 0, 255};
    EXPECT_NE(left, right);
}

TEST(MeshDataTest, IsComplete_AcceptsOneWholeTriangle)
{
    EXPECT_TRUE(oneTriangle().isComplete());
}

TEST(MeshDataTest, IsComplete_RejectsNoIndicesAtAll)
{
    MeshData mesh = oneTriangle();
    mesh.indices.clear();

    EXPECT_FALSE(mesh.isComplete());
}

TEST(MeshDataTest, IsComplete_RejectsAPartialTriangle)
{
    MeshData mesh = oneTriangle();
    mesh.indices.pop_back();

    EXPECT_FALSE(mesh.isComplete());
}

TEST(MeshDataTest, IsComplete_RejectsAnIndexPastTheLastVertex)
{
    MeshData mesh = oneTriangle();
    mesh.indices.back() = 3U;

    EXPECT_FALSE(mesh.isComplete());
}

TEST(MeshDataTest, TriangleCount_IsThreeIndicesPerTriangle)
{
    EXPECT_EQ(1U, oneTriangle().triangleCount());
    EXPECT_EQ(0U, MeshData{}.triangleCount());
}
