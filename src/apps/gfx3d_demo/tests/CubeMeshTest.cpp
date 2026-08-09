#include <gtest/gtest.h>
#include <glm/geometric.hpp>

#include <cmath>
#include <cstddef>
#include <set>
#include <tuple>

#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/MeshData.hpp>

#include "antwika/gfx3d_demo/CubeMesh.hpp"

using antwika::gfx::Vec3;
using antwika::gfx3d_demo::cubeMesh;

namespace
{
    constexpr std::size_t kFaces = 6;
    constexpr std::size_t kVerticesPerFace = 4;
    constexpr std::size_t kTrianglesPerFace = 2;

    std::tuple<float, float, float> keyOf(Vec3 value)
    {
        return {value.x, value.y, value.z};
    }
}

TEST(CubeMeshTest, CubeMesh_IsCompleteGeometry)
{
    EXPECT_TRUE(cubeMesh().isComplete());
}

TEST(CubeMeshTest, CubeMesh_HasItsOwnQuadPerFace)
{
    const auto mesh = cubeMesh();

    EXPECT_EQ(mesh.vertices.size(), kFaces * kVerticesPerFace);
    EXPECT_EQ(mesh.triangleCount(), kFaces * kTrianglesPerFace);
}

TEST(CubeMeshTest, CubeMesh_GivesEachFaceItsOwnNormal)
{
    const auto mesh = cubeMesh();

    std::set<std::tuple<float, float, float>> normals;

    for (const auto &vertex : mesh.vertices)
    {
        normals.insert(keyOf(vertex.normal));
    }

    EXPECT_EQ(normals.size(), kFaces);
}

TEST(CubeMeshTest, CubeMesh_GivesEachFaceItsOwnColour)
{
    const auto mesh = cubeMesh();

    std::set<std::tuple<int, int, int>> colors;

    for (const auto &vertex : mesh.vertices)
    {
        colors.insert(
            {vertex.color.red, vertex.color.green, vertex.color.blue});
    }

    EXPECT_EQ(colors.size(), kFaces);
}

TEST(CubeMeshTest, CubeMesh_KeepsEveryCornerOnTheUnitCube)
{
    const auto mesh = cubeMesh();

    for (const auto &vertex : mesh.vertices)
    {
        EXPECT_FLOAT_EQ(std::abs(vertex.position.x), 0.5F);
        EXPECT_FLOAT_EQ(std::abs(vertex.position.y), 0.5F);
        EXPECT_FLOAT_EQ(std::abs(vertex.position.z), 0.5F);
    }
}

TEST(CubeMeshTest, CubeMesh_WindsEveryTriangleOutwards)
{
    const auto mesh = cubeMesh();

    for (std::size_t triangle = 0; triangle < mesh.triangleCount();
         ++triangle)
    {
        const auto &a =
            mesh.vertices[mesh.indices[(triangle * 3) + 0]].position;
        const auto &b =
            mesh.vertices[mesh.indices[(triangle * 3) + 1]].position;
        const auto &c =
            mesh.vertices[mesh.indices[(triangle * 3) + 2]].position;

        const Vec3 face = glm::cross(b - a, c - b);
        const Vec3 outward = (a + b + c) / 3.0F;

        EXPECT_GT(glm::dot(face, outward), 0.0F) << triangle;
    }
}

TEST(CubeMeshTest, CubeMesh_AgreesWithTheNormalItDeclares)
{
    const auto mesh = cubeMesh();

    for (std::size_t triangle = 0; triangle < mesh.triangleCount();
         ++triangle)
    {
        const auto &first = mesh.vertices[mesh.indices[triangle * 3]];
        const auto &b =
            mesh.vertices[mesh.indices[(triangle * 3) + 1]].position;
        const auto &c =
            mesh.vertices[mesh.indices[(triangle * 3) + 2]].position;

        const Vec3 face =
            glm::normalize(glm::cross(b - first.position, c - b));

        EXPECT_FLOAT_EQ(glm::dot(face, first.normal), 1.0F) << triangle;
    }
}
