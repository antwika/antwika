#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/gfx/WindowEvent.hpp>
#include <antwika/gfx/mocks/MockGfxBackend.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>
#include <antwika/gfx/mocks/MockWindow.hpp>

#include "antwika/gfx_demo/DemoLoop.hpp"
#include "antwika/gfx_demo/DemoScene.hpp"

using antwika::gfx::Bitmap;
using antwika::gfx::CloseRequested;
using antwika::gfx::ITexture;
using antwika::gfx::IWindow;
using antwika::gfx::Resized;
using antwika::gfx::Size;
using antwika::gfx::WindowDesc;
using antwika::gfx::WindowEvent;
using antwika::gfx::WindowId;
using antwika::gfx::mocks::MockGfxBackend;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockTexture;
using antwika::gfx::mocks::MockWindow;
using antwika::gfx_demo::DemoLoop;
using antwika::gfx_demo::DemoScene;
using ::testing::ByMove;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace
{
    /**
     * @brief Holds the mock window a backend is about to hand out, so a
     * test can still assert on it after ownership has moved away.
     */
    struct DemoFixture
    {
        NiceMock<MockGfxBackend> backend;
        NiceMock<MockRenderer> renderer;
        MockWindow *window = nullptr;

        static constexpr WindowId kOurWindow{1};
        static constexpr WindowId kSomeoneElsesWindow{99};

        // A real 1x1 bitmap for the loop to hand to createTexture.
        static Bitmap logo()
        {
            return Bitmap{
                .size = {.width = 1, .height = 1},
                .pixels = std::vector<std::uint8_t>{1, 2, 3, 255}};
        }

        void expectOneWindow(bool open)
        {
            auto owned = std::make_unique<NiceMock<MockWindow>>();
            window = owned.get();

            ON_CALL(*window, id()).WillByDefault(Return(kOurWindow));
            ON_CALL(*window, isOpen()).WillByDefault(Return(open));
            ON_CALL(*window, renderer()).WillByDefault(ReturnRef(renderer));
            ON_CALL(*window, size())
                .WillByDefault(Return(Size{.width = 700, .height = 400}));

            EXPECT_CALL(backend, createWindow(::testing::_))
                .WillOnce(Return(ByMove(
                    std::unique_ptr<IWindow>(std::move(owned)))));

            // WillOnce is the assertion here.
            // The upload happens once a run, not once a frame.
            EXPECT_CALL(renderer, createTexture(::testing::_))
                .WillOnce(Return(ByMove(
                    std::unique_ptr<ITexture>(
                        std::make_unique<NiceMock<MockTexture>>()))));
        }
    };
} // namespace

TEST(DemoLoopTest, Run_DrawsAndPresentsOncePerFrame)
{
    DemoFixture fixture;
    fixture.expectOneWindow(true);

    ON_CALL(fixture.backend, pollEvent())
        .WillByDefault(Return(std::nullopt));

    EXPECT_CALL(fixture.renderer, present()).Times(2);
    EXPECT_CALL(fixture.renderer, clear(::testing::_)).Times(2);

    const DemoScene scene;
    DemoLoop loop(fixture.backend, scene);

    loop.run(WindowDesc{.title = "Antwika"}, DemoFixture::logo(), 2);
}

TEST(DemoLoopTest, Run_ClosesTheWindowWhenTheBackendReportsACloseRequest)
{
    DemoFixture fixture;
    fixture.expectOneWindow(false);

    EXPECT_CALL(fixture.backend, pollEvent())
        .WillOnce(Return(WindowEvent{
            .window = DemoFixture::kOurWindow,
            .payload = CloseRequested{}}))
        .WillRepeatedly(Return(std::nullopt));

    EXPECT_CALL(fixture.renderer, present()).Times(0);

    const DemoScene scene;
    DemoLoop loop(fixture.backend, scene);

    // Once for the close request and once on the way out.
    // Closing an already-closed window is part of IWindow's contract.
    EXPECT_CALL(*fixture.window, close()).Times(2);

    loop.run(WindowDesc{.title = "Antwika"}, DemoFixture::logo(), 10);
}

TEST(DemoLoopTest, Run_KeepsDrawingThroughEventsThatAreNotCloseRequests)
{
    DemoFixture fixture;
    fixture.expectOneWindow(true);

    EXPECT_CALL(fixture.backend, pollEvent())
        .WillOnce(Return(WindowEvent{
            .window = DemoFixture::kOurWindow,
            .payload = Resized{.size = {.width = 700, .height = 400}}}))
        .WillRepeatedly(Return(std::nullopt));

    EXPECT_CALL(fixture.renderer, present()).Times(1);

    const DemoScene scene;
    DemoLoop loop(fixture.backend, scene);

    loop.run(WindowDesc{.title = "Antwika"}, DemoFixture::logo(), 1);
}

TEST(DemoLoopTest, Run_IgnoresACloseRequestForSomebodyElsesWindow)
{
    DemoFixture fixture;
    fixture.expectOneWindow(true);

    EXPECT_CALL(fixture.backend, pollEvent())
        .WillOnce(Return(WindowEvent{
            .window = DemoFixture::kSomeoneElsesWindow,
            .payload = CloseRequested{}}))
        .WillRepeatedly(Return(std::nullopt));

    EXPECT_CALL(fixture.renderer, present()).Times(1);

    const DemoScene scene;
    DemoLoop loop(fixture.backend, scene);

    // Only the one on the way out, never one caused by that event.
    EXPECT_CALL(*fixture.window, close()).Times(1);

    loop.run(WindowDesc{.title = "Antwika"}, DemoFixture::logo(), 1);
}

TEST(DemoLoopTest, Run_WithoutAFrameCapDrawsUntilTheWindowCloses)
{
    DemoFixture fixture;
    fixture.expectOneWindow(true);

    // Two frames go by, then the user closes the window.
    EXPECT_CALL(fixture.backend, pollEvent())
        .WillOnce(Return(std::nullopt))
        .WillOnce(Return(std::nullopt))
        .WillOnce(Return(WindowEvent{
            .window = DemoFixture::kOurWindow,
            .payload = CloseRequested{}}))
        .WillRepeatedly(Return(std::nullopt));

    EXPECT_CALL(*fixture.window, isOpen())
        .WillOnce(Return(true))
        .WillOnce(Return(true))
        .WillRepeatedly(Return(false));

    EXPECT_CALL(fixture.renderer, present()).Times(2);

    const DemoScene scene;
    DemoLoop loop(fixture.backend, scene);

    loop.run(WindowDesc{.title = "Antwika"}, DemoFixture::logo(), std::nullopt);
}

TEST(DemoLoopTest, Run_UploadsTheTextureOnceForTheWholeRun)
{
    DemoFixture fixture;
    fixture.expectOneWindow(true);

    ON_CALL(fixture.backend, pollEvent())
        .WillByDefault(Return(std::nullopt));

    // Three frames drawn.
    // The WillOnce in expectOneWindow says one upload even so.
    EXPECT_CALL(fixture.renderer, present()).Times(3);

    const DemoScene scene;
    DemoLoop loop(fixture.backend, scene);

    loop.run(WindowDesc{.title = "Antwika"}, DemoFixture::logo(), 3);
}

TEST(DemoLoopTest, Run_OpensAndClosesTheWindowEvenWithNoFrames)
{
    DemoFixture fixture;
    fixture.expectOneWindow(true);

    EXPECT_CALL(fixture.backend, pollEvent()).Times(0);
    EXPECT_CALL(fixture.renderer, present()).Times(0);

    const DemoScene scene;
    DemoLoop loop(fixture.backend, scene);

    EXPECT_CALL(*fixture.window, close()).Times(1);

    loop.run(WindowDesc{.title = "Antwika"}, DemoFixture::logo(), 0);
}
