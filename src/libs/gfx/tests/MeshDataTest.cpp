#include <gtest/gtest.h>

#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/GfxError.hpp"
#include "antwika/gfx/Math3D.hpp"
#include "antwika/gfx/MeshData.hpp"

using antwika::gfx::Color;
using antwika::gfx::GfxError;
using antwika::gfx::MeshData;
using antwika::gfx::splitMesh;
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

    MeshData twoTriangles()
    {
        MeshData mesh = oneTriangle();
        mesh.indices.push_back(0U);
        mesh.indices.push_back(2U);
        mesh.indices.push_back(1U);
        return mesh;
    }
}

TEST(MeshDataTest, OperatorEquals_ComparesTheVerticesAndTheIndices)
{
    const MeshData leftData = oneTriangle();

    EXPECT_EQ(leftData, oneTriangle());
    EXPECT_NE(leftData, twoTriangles());

    MeshData movedData = oneTriangle();
    movedData.vertices[0].position = Vec3{9.0F, 9.0F, 9.0F};

    EXPECT_NE(leftData, movedData);
}

TEST(Vertex3DTest, OperatorEquals_ComparesEveryField)
{
    const Vertex3D leftVertex;
    Vertex3D rightVertex;

    EXPECT_EQ(leftVertex, rightVertex);

    rightVertex = leftVertex;
    rightVertex.position = Vec3{1.0F, 0.0F, 0.0F};
    EXPECT_NE(leftVertex, rightVertex);

    rightVertex = leftVertex;
    rightVertex.normal = Vec3{0.0F, 1.0F, 0.0F};
    EXPECT_NE(leftVertex, rightVertex);

    rightVertex = leftVertex;
    rightVertex.texCoordinate = Vec2{1.0F, 0.0F};
    EXPECT_NE(leftVertex, rightVertex);

    rightVertex = leftVertex;
    rightVertex.color = Color{0, 0, 0, 255};
    EXPECT_NE(leftVertex, rightVertex);
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
    EXPECT_EQ(2U, twoTriangles().triangleCount());
    EXPECT_EQ(0U, MeshData{}.triangleCount());
}

TEST(MeshDataTest, SplitMesh_LeavesAMeshWithinTheCountAsOnePiece)
{
    const auto pieces = splitMesh(twoTriangles(), 64U);

    ASSERT_EQ(pieces.size(), 1U);
    EXPECT_TRUE(pieces.front().isComplete());
    EXPECT_EQ(pieces.front().triangleCount(), 2U);
}

TEST(MeshDataTest, SplitMesh_KeepsEveryTriangleWholeInOnePiece)
{
    const auto wholeMesh = twoTriangles();
    const auto pieces = splitMesh(wholeMesh, 3U);

    ASSERT_EQ(pieces.size(), 2U);

    std::size_t triangles = 0;

    for (const auto &piece : pieces)
    {
        EXPECT_TRUE(piece.isComplete());
        EXPECT_LE(piece.vertices.size(), 3U);
        triangles += piece.triangleCount();
    }

    EXPECT_EQ(triangles, wholeMesh.triangleCount());
}

TEST(MeshDataTest, SplitMesh_DrawsTheSameCornersItWasGiven)
{
    const auto wholeMesh = twoTriangles();
    const auto pieces = splitMesh(wholeMesh, 3U);

    std::vector<Vertex3D> drawnVertices;

    for (const auto &piece : pieces)
    {
        for (const auto index : piece.indices)
        {
            drawnVertices.push_back(piece.vertices[index]);
        }
    }

    std::vector<Vertex3D> wantedVertices;

    for (const auto index : wholeMesh.indices)
    {
        wantedVertices.push_back(wholeMesh.vertices[index]);
    }

    EXPECT_EQ(drawnVertices, wantedVertices);
}

TEST(MeshDataTest, SplitMesh_HoldsACornerSharedWithinAPieceOnlyOnce)
{
    MeshData sharedData = twoTriangles();
    sharedData.indices = {0U, 1U, 2U, 0U, 1U, 2U};

    const auto pieces = splitMesh(sharedData, 64U);

    ASSERT_EQ(pieces.size(), 1U);
    EXPECT_EQ(pieces.front().vertices.size(), 3U);
    EXPECT_EQ(pieces.front().triangleCount(), 2U);
}

TEST(MeshDataTest, SplitMesh_TurnsAwayAMeshThatDoesNotIndexItsVertices)
{
    MeshData brokenData = oneTriangle();
    brokenData.indices = {0U, 1U, 9U};

    EXPECT_THROW((void)splitMesh(brokenData, 64U), GfxError);
}

TEST(MeshDataTest, SplitMesh_TurnsAwayAPieceTooSmallForATriangle)
{
    EXPECT_THROW((void)splitMesh(oneTriangle(), 2U), GfxError);
}
