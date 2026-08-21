#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/IMesh.hpp>
#include <antwika/gfx/IRenderTarget.hpp>
#include <antwika/gfx/IShader.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/gfx/fakes/FakeSizedTarget.hpp>
#include <antwika/gfx/mocks/MockMesh.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockShader.hpp>

#include "antwika/render/ScenePass.hpp"

using antwika::render::ScenePass;
using antwika::gfx::IMesh;
using antwika::gfx::IRenderTarget;
using antwika::gfx::IShader;
using antwika::gfx::ITexture;
using antwika::gfx::ShaderSource;
using antwika::gfx::Size;
using antwika::gfx::ViewportRenderer;
using antwika::gfx::fakes::FakeSizedTarget;
using antwika::gfx::mocks::MockMesh;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockShader;
using ::testing::NiceMock;

namespace
{
    constexpr Size kWindowSize{.width = 960, .height = 540};

    constexpr Size kCanvasSize{.width = 480, .height = 270};

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
        ON_CALL(innerRenderer, createMesh(::testing::_))
            .WillByDefault(
                []([[maybe_unused]] const antwika::gfx::MeshData &data)
                {
                    return std::unique_ptr<IMesh>{
                        std::make_unique<NiceMock<MockMesh>>()};
                });
        ON_CALL(innerRenderer, createRenderTarget(::testing::_))
            .WillByDefault(
                [](const antwika::gfx::RenderTargetSpec &spec)
                {
                    return std::unique_ptr<IRenderTarget>{
                        std::make_unique<FakeSizedTarget>(spec.size)};
                });
    }
}

TEST(ScenePassTest, Open_TakesUpTheBloomPassAndTheQuadItIsLaidOver)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutResources(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    ScenePass pass;

    EXPECT_CALL(innerRenderer, createShader).Times(1);
    EXPECT_CALL(innerRenderer, createMesh).Times(1);

    pass.open(viewportRenderer, ShaderSource{});
}

TEST(ScenePassTest, Draw_DrawsThePileOnceForTheGlowAndOnceForTheScene)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutResources(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    NiceMock<MockShader> voxelShader;
    ScenePass pass;
    auto piles = 0;
    auto overs = 0;

    pass.open(viewportRenderer, ShaderSource{});
    pass.draw(
        viewportRenderer,
        voxelShader,
        antwika::gfx::Color{},
        [&piles] { ++piles; },
        [&overs] { ++overs; });

    EXPECT_EQ(piles, 2);
    EXPECT_EQ(overs, 1);
}

TEST(ScenePassTest, Draw_MakesItsTargetsOnceAndKeepsThem)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutResources(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    NiceMock<MockShader> voxelShader;
    ScenePass pass;
    const auto nothing = [] {};

    pass.open(viewportRenderer, ShaderSource{});
    pass.draw(
        viewportRenderer,
        voxelShader,
        antwika::gfx::Color{},
        nothing,
        nothing);

    EXPECT_CALL(innerRenderer, createRenderTarget).Times(0);

    pass.draw(
        viewportRenderer,
        voxelShader,
        antwika::gfx::Color{},
        nothing,
        nothing);
}

TEST(ScenePassTest, Draw_AsksForASceneTargetTheSizeOfAResizedWindow)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutResources(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    NiceMock<MockShader> voxelShader;
    ScenePass pass;
    const auto nothing = [] {};

    pass.open(viewportRenderer, ShaderSource{});
    pass.draw(
        viewportRenderer,
        voxelShader,
        antwika::gfx::Color{},
        nothing,
        nothing);
    viewportRenderer.resize(Size{.width = 1920, .height = 1080});

    EXPECT_CALL(innerRenderer, createRenderTarget).Times(1);

    pass.draw(
        viewportRenderer,
        voxelShader,
        antwika::gfx::Color{},
        nothing,
        nothing);
}
