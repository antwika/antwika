#include <gtest/gtest.h>

#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/MeshBox.hpp>
#include <antwika/gfx/MeshData.hpp>
#include <antwika/gfx/Vertex3D.hpp>

using antwika::gfx::getMeshBox;
using antwika::gfx::getSpanFromBox;
using antwika::gfx::isBoxBeyond;
using antwika::gfx::isBoxOutside;
using antwika::gfx::MeshBox;
using antwika::gfx::MeshData;
using antwika::gfx::Vec3;
using antwika::gfx::Vertex3D;

namespace
{
    [[nodiscard]] MeshData getMeshOf(const std::vector<Vec3> &positions)
    {
        MeshData mesh;

        for (const auto position : positions)
        {
            mesh.vertices.push_back(Vertex3D{.position = position});
        }

        return mesh;
    }
}

TEST(MeshBoxTest, MeshBox_WrapsEveryCornerTheMeshReaches)
{
    const auto box = getMeshBox(
        getMeshOf(
            {Vec3{1.0F, -2.0F, 3.0F},
             Vec3{-4.0F, 5.0F, 0.0F},
             Vec3{2.0F, 2.0F, -6.0F}}));

    EXPECT_EQ(box.lowPosition, (Vec3{-4.0F, -2.0F, -6.0F}));
    EXPECT_EQ(box.highPosition, (Vec3{2.0F, 5.0F, 3.0F}));
}

TEST(MeshBoxTest, MeshBox_SitsAtTheOriginForAMeshWithNoVertices)
{
    const auto box = getMeshBox(MeshData{});

    EXPECT_EQ(box.lowPosition, Vec3{});
    EXPECT_EQ(box.highPosition, Vec3{});
}

TEST(MeshBoxTest, SpanFromBox_ReadsNothingFromWithinTheBox)
{
    const MeshBox box{
        .lowPosition = Vec3{-1.0F, -1.0F, -1.0F},
        .highPosition = Vec3{1.0F, 1.0F, 1.0F}};

    EXPECT_FLOAT_EQ(getSpanFromBox(box, Vec3{}), 0.0F);
    EXPECT_FLOAT_EQ(getSpanFromBox(box, Vec3{1.0F, 0.0F, 0.0F}), 0.0F);
}

TEST(MeshBoxTest, SpanFromBox_ReachesTheNearestFaceNotTheMiddle)
{
    const MeshBox box{
        .lowPosition = Vec3{2.0F, 0.0F, 0.0F},
        .highPosition = Vec3{4.0F, 0.0F, 0.0F}};

    EXPECT_FLOAT_EQ(getSpanFromBox(box, Vec3{}), 2.0F);
}

TEST(MeshBoxTest, IsBoxBeyond_HoldsOnlyPastTheReachItIsGiven)
{
    const MeshBox box{
        .lowPosition = Vec3{10.0F, 0.0F, 0.0F},
        .highPosition = Vec3{11.0F, 0.0F, 0.0F}};

    EXPECT_TRUE(isBoxBeyond(box, Vec3{}, 5.0F));
    EXPECT_FALSE(isBoxBeyond(box, Vec3{}, 20.0F));
    EXPECT_FALSE(isBoxBeyond(box, Vec3{}, 10.0F));
}

namespace
{
    [[nodiscard]] antwika::gfx::Mat4 getLookingDownZ()
    {
        return antwika::gfx::Camera3D{
            Vec3{0.0F, 0.0F, 0.0F},
            Vec3{0.0F, 0.0F, -1.0F},
            Vec3{0.0F, 1.0F, 0.0F},
            antwika::gfx::Orthographic{
                .halfWidth = 4.0F,
                .halfHeight = 4.0F,
                .nearPlane = 0.1F,
                .farPlane = 100.0F}}
            .getViewProjection();
    }
}

TEST(MeshBoxTest, IsBoxOutside_KeepsABoxTheCameraLooksStraightAt)
{
    const MeshBox box{
        .lowPosition = Vec3{-1.0F, -1.0F, -11.0F},
        .highPosition = Vec3{1.0F, 1.0F, -9.0F}};

    EXPECT_FALSE(isBoxOutside(box, getLookingDownZ()));
}

TEST(MeshBoxTest, IsBoxOutside_DropsABoxWideOfTheCamera)
{
    const MeshBox box{
        .lowPosition = Vec3{40.0F, -1.0F, -11.0F},
        .highPosition = Vec3{42.0F, 1.0F, -9.0F}};

    EXPECT_TRUE(isBoxOutside(box, getLookingDownZ()));
}

TEST(MeshBoxTest, IsBoxOutside_DropsABoxBehindTheCamera)
{
    const MeshBox box{
        .lowPosition = Vec3{-1.0F, -1.0F, 9.0F},
        .highPosition = Vec3{1.0F, 1.0F, 11.0F}};

    EXPECT_TRUE(isBoxOutside(box, getLookingDownZ()));
}

TEST(MeshBoxTest, IsBoxOutside_KeepsABoxTheEdgeOfTheViewCuts)
{
    const MeshBox box{
        .lowPosition = Vec3{3.0F, -1.0F, -11.0F},
        .highPosition = Vec3{6.0F, 1.0F, -9.0F}};

    EXPECT_FALSE(isBoxOutside(box, getLookingDownZ()));
}
