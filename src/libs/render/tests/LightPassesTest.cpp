#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <set>
#include <vector>

#include <antwika/gfx/CubeFace.hpp>
#include <antwika/gfx/IMesh.hpp>
#include <antwika/gfx/IRenderTarget.hpp>
#include <antwika/gfx/IShader.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/MeshBox.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/gfx/fakes/FakeBareTarget.hpp>
#include <antwika/gfx/mocks/MockMesh.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockShader.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>
#include <antwika/light/ActiveLight.hpp>
#include <antwika/voxel/VoxelCell.hpp>

#include "antwika/render/LightPasses.hpp"

using antwika::render::LightPasses;
using antwika::render::MeshPiece;
using antwika::gfx::MeshBox;
using antwika::voxel::Voxels;
using antwika::gfx::IMesh;
using antwika::gfx::IRenderTarget;
using antwika::gfx::IShader;
using antwika::gfx::ITexture;
using antwika::gfx::ShaderSource;
using antwika::gfx::Size;
using antwika::gfx::Vec3;
using antwika::gfx::ViewportRenderer;
using antwika::gfx::fakes::FakeBareTarget;
using antwika::gfx::mocks::MockMesh;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockShader;
using antwika::gfx::mocks::MockTexture;
using antwika::light::ActiveLight;
using antwika::voxel::VoxelCell;
using antwika::voxel::voxelsOf;
using ::testing::NiceMock;
using antwika::voxel::VoxelPosition;

namespace
{
    constexpr Size kWindowSize{.width = 960, .height = 540};

    constexpr Size kCanvasSize{.width = 480, .height = 270};

    constexpr int kFacesOfALamp = 6;

    void handsOutResources(NiceMock<MockRenderer> &innerRenderer)
    {
        ON_CALL(innerRenderer, createShader(::testing::_))
            .WillByDefault(
                []([[maybe_unused]] const antwika::gfx::ShaderSource
                       &source)
                {
                    return std::unique_ptr<IShader>{
                        std::make_unique<NiceMock<MockShader>>()};
                });
        ON_CALL(innerRenderer, createRenderTarget(::testing::_))
            .WillByDefault(
                []([[maybe_unused]] const antwika::gfx::
                       RenderTargetSpec &spec)
                {
                    return std::unique_ptr<IRenderTarget>{
                        std::make_unique<FakeBareTarget>()};
                });
        ON_CALL(innerRenderer, createTexture(::testing::_))
            .WillByDefault(
                []([[maybe_unused]] const antwika::gfx::Bitmap &bitmap)
                {
                    return std::unique_ptr<ITexture>{
                        std::make_unique<NiceMock<MockTexture>>()};
                });
        ON_CALL(innerRenderer, createMesh(::testing::_))
            .WillByDefault(
                []([[maybe_unused]] const antwika::gfx::MeshData &data)
                {
                    return std::unique_ptr<IMesh>{
                        std::make_unique<NiceMock<MockMesh>>()};
                });
    }

    [[nodiscard]] std::vector<MeshPiece> getPieceInBox(const MeshBox box)
    {
        std::vector<MeshPiece> meshes;

        meshes.push_back(
            MeshPiece{
                .mesh = std::make_unique<NiceMock<MockMesh>>(), .box = box});

        return meshes;
    }

    [[nodiscard]] std::vector<MeshPiece> getOnePiece()
    {
        return getPieceInBox(
            MeshBox{
                .lowPosition = Vec3{-8.0F, -8.0F, -8.0F},
                .highPosition = Vec3{8.0F, 8.0F, 8.0F}});
    }
}

TEST(LightPassesTest, Open_TakesUpTheShadowPassAndTheAtlasItBakesInto)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutResources(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    LightPasses passes;

    EXPECT_CALL(innerRenderer, createShader).Times(1);
    EXPECT_CALL(innerRenderer, createRenderTarget).Times(1);

    passes.open(viewportRenderer, ShaderSource{});
}

TEST(LightPassesTest, Hide_DrawsTheMaskOnceForTheSameCubesAndPlace)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutResources(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    LightPasses passes;
    const auto behindCells = voxelsOf({VoxelCell{.position = {.x = 1,
        .z = 2}}});

    passes.open(viewportRenderer, ShaderSource{});
    passes.hide(viewportRenderer, behindCells, VoxelPosition{});

    EXPECT_EQ(passes.getHiddenVoxels(), behindCells);
    EXPECT_NE(passes.getHiding(), nullptr);

    EXPECT_CALL(innerRenderer, createTexture).Times(0);
    EXPECT_CALL(innerRenderer, updateTexture).Times(0);

    passes.hide(viewportRenderer, behindCells, VoxelPosition{});
}

TEST(LightPassesTest, Hide_DrawsTheMaskAfreshWhereTheCubesChanged)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutResources(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    LightPasses passes;

    passes.open(viewportRenderer, ShaderSource{});
    passes.hide(viewportRenderer, voxelsOf({VoxelCell{.position = {.x = 1,
        .z = 2}}}),
        VoxelPosition{});

    EXPECT_CALL(innerRenderer, updateTexture).Times(1);

    passes.hide(viewportRenderer, voxelsOf({VoxelCell{.position = {.x = 3,
        .z = 4}}}),
        VoxelPosition{});

    EXPECT_EQ(passes.getHiddenVoxels().size(), 1U);
}

TEST(LightPassesTest, BakeLamps_DrawsEveryFaceOfALampThatHasMoved)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutResources(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    LightPasses passes;
    const auto pile = getOnePiece();
    const std::vector<ActiveLight> activeLights{
        ActiveLight{.position = Vec3{1.0F, 2.0F, 3.0F}}};

    passes.open(viewportRenderer, ShaderSource{});

    EXPECT_CALL(innerRenderer, drawMesh).Times(kFacesOfALamp);

    passes.bakeLamps(viewportRenderer, pile, activeLights);

    EXPECT_EQ(passes.getLamps().size(), 1U);
    EXPECT_EQ(passes.getLamps()[0].position, activeLights.front().position);
}

TEST(LightPassesTest, BakeLamps_LeavesALampThatHasNotMovedAlone)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutResources(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    LightPasses passes;
    const auto pile = getOnePiece();
    const std::vector<ActiveLight> activeLights{
        ActiveLight{.position = Vec3{1.0F, 2.0F, 3.0F}}};

    passes.open(viewportRenderer, ShaderSource{});
    passes.bakeLamps(viewportRenderer, pile, activeLights);

    EXPECT_CALL(innerRenderer, drawMesh).Times(0);

    passes.bakeLamps(viewportRenderer, pile, activeLights);
}

TEST(LightPassesTest, Forget_BakesTheLampsAfreshThoughNoneHasMoved)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutResources(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    LightPasses passes;
    const auto pile = getOnePiece();
    const std::vector<ActiveLight> activeLights{
        ActiveLight{.position = Vec3{1.0F, 2.0F, 3.0F}}};

    passes.open(viewportRenderer, ShaderSource{});
    passes.bakeLamps(viewportRenderer, pile, activeLights);
    passes.forget();

    EXPECT_CALL(innerRenderer, drawMesh).Times(kFacesOfALamp);

    passes.bakeLamps(viewportRenderer, pile, activeLights);
}

TEST(LightPassesTest, BakeLamps_LeavesOutAPiecePastTheLampsReach)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutResources(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    LightPasses passes;
    const auto span = antwika::light::kLampFarPlane * 4.0F;
    const auto pile = getPieceInBox(
        MeshBox{
            .lowPosition = Vec3{span, span, span},
            .highPosition = Vec3{span + 1.0F, span + 1.0F, span + 1.0F}});
    const std::vector<ActiveLight> activeLights{
        ActiveLight{.position = Vec3{}}};

    passes.open(viewportRenderer, ShaderSource{});

    EXPECT_CALL(innerRenderer, drawMesh).Times(0);

    passes.bakeLamps(viewportRenderer, pile, activeLights);
}

TEST(LightPassesTest, BakeLamps_LeavesOutAPieceTheBakeLooksAwayFrom)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutResources(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    LightPasses passes;
    const auto pile = getPieceInBox(
        MeshBox{
            .lowPosition = Vec3{10.0F, -1.0F, -1.0F},
            .highPosition = Vec3{12.0F, 1.0F, 1.0F}});
    const std::vector<ActiveLight> activeLights{
        ActiveLight{.position = Vec3{}}};

    passes.open(viewportRenderer, ShaderSource{});

    EXPECT_CALL(innerRenderer, drawMesh).Times(kFacesOfALamp - 1);

    passes.bakeLamps(viewportRenderer, pile, activeLights);
}

TEST(LightPassesTest, BakeLamps_DrawsNothingWithNoPileToDraw)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutResources(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    LightPasses passes;
    const std::vector<ActiveLight> activeLights{ActiveLight{}};

    passes.open(viewportRenderer, ShaderSource{});

    EXPECT_CALL(innerRenderer, drawMesh).Times(0);

    passes.bakeLamps(viewportRenderer, {}, activeLights);

    EXPECT_TRUE(passes.getLamps().empty());
}

