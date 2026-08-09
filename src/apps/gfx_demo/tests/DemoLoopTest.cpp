#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include <antwika/time/fakes/FakeSleeper.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/gfx/WindowEvent.hpp>
#include <antwika/gfx/mocks/MockGfxBackend.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>
#include <antwika/gfx/mocks/MockWindow.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/input/Position.hpp>
#include <antwika/input/fakes/FakeInputBackend.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/Theme.hpp>
#include <antwika/ui/WidgetId.hpp>

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
using antwika::input::InputEvent;
using antwika::input::MouseButton;
using antwika::input::KeyPressed;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerButtonReleased;
using antwika::input::PointerMoved;
using antwika::input::Position;
using antwika::input::fakes::FakeInputBackend;
using antwika::ui::Pointer;
using antwika::ui::WidgetId;
namespace widgets = antwika::gfx_demo::widgets;
using ::testing::ByMove;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace
{
    constexpr std::chrono::milliseconds kTestFramePeriod{1};

    constexpr Size kCanvas{.width = 700, .height = 400};

    [[nodiscard]] Position positionOn(WidgetId id)
    {
        const DemoScene scene;

        for (std::int32_t y = 0;
             y < static_cast<std::int32_t>(kCanvas.height);
             y += 4)
        {
            for (std::int32_t x = 0;
                 x < static_cast<std::int32_t>(kCanvas.width);
                 x += 4)
            {
                const Pointer pointer{
                    .position = antwika::gfx::Point{.x = x, .y = y}};

                if (scene.describe(kCanvas, pointer).interactions.hovered
                    == id)
                {
                    return Position{.x = x, .y = y};
                }
            }
        }

        ADD_FAILURE() << "no pixel of the canvas hovers a widget";

        return Position{};
    }

    [[nodiscard]] std::vector<InputEvent> clickAt(Position position)
    {
        return {
            PointerMoved{.position = position},
            PointerButtonPressed{
                .button = MouseButton::Left, .position = position}};
    }

    struct DemoFixture final
    {
        NiceMock<MockGfxBackend> backend;
        NiceMock<MockRenderer> renderer;
        MockWindow *window = nullptr;

        static constexpr WindowId kOurWindow{1};
        static constexpr WindowId kSomeoneElsesWindow{99};

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
            ON_CALL(*window, size()).WillByDefault(Return(kCanvas));

            EXPECT_CALL(backend, createWindow(::testing::_))
                .WillOnce(Return(ByMove(
                    std::unique_ptr<IWindow>(std::move(owned)))));

            EXPECT_CALL(renderer, createTexture(::testing::_))
                .WillOnce(Return(ByMove(
                    std::unique_ptr<ITexture>(
                        std::make_unique<NiceMock<MockTexture>>()))));
        }
    };
}

TEST(DemoLoopTest, Run_DrawsAndPresentsOncePerFrame)
{
    DemoFixture fixture;
    fixture.expectOneWindow(true);

    ON_CALL(fixture.backend, pollEvent())
        .WillByDefault(Return(std::nullopt));

    EXPECT_CALL(fixture.renderer, present()).Times(2);
    EXPECT_CALL(fixture.renderer, clear(::testing::_)).Times(2);

    const DemoScene scene;
    FakeInputBackend input;
    antwika::time::fakes::FakeSleeper sleeper;
    DemoLoop loop(
        fixture.backend, input, scene, sleeper, kTestFramePeriod);

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
    FakeInputBackend input;
    antwika::time::fakes::FakeSleeper sleeper;
    DemoLoop loop(
        fixture.backend, input, scene, sleeper, kTestFramePeriod);

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
    FakeInputBackend input;
    antwika::time::fakes::FakeSleeper sleeper;
    DemoLoop loop(
        fixture.backend, input, scene, sleeper, kTestFramePeriod);

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
    FakeInputBackend input;
    antwika::time::fakes::FakeSleeper sleeper;
    DemoLoop loop(
        fixture.backend, input, scene, sleeper, kTestFramePeriod);

    EXPECT_CALL(*fixture.window, close()).Times(1);

    loop.run(WindowDesc{.title = "Antwika"}, DemoFixture::logo(), 1);
}

TEST(DemoLoopTest, Run_WithoutAFrameCapDrawsUntilTheWindowCloses)
{
    DemoFixture fixture;
    fixture.expectOneWindow(true);

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
    FakeInputBackend input;
    antwika::time::fakes::FakeSleeper sleeper;
    DemoLoop loop(
        fixture.backend, input, scene, sleeper, kTestFramePeriod);

    loop.run(WindowDesc{.title = "Antwika"}, DemoFixture::logo(), std::nullopt);
}

TEST(DemoLoopTest, Run_UploadsTheTextureOnceForTheWholeRun)
{
    DemoFixture fixture;
    fixture.expectOneWindow(true);

    ON_CALL(fixture.backend, pollEvent())
        .WillByDefault(Return(std::nullopt));

    EXPECT_CALL(fixture.renderer, present()).Times(3);

    const DemoScene scene;
    FakeInputBackend input;
    antwika::time::fakes::FakeSleeper sleeper;
    DemoLoop loop(
        fixture.backend, input, scene, sleeper, kTestFramePeriod);

    loop.run(WindowDesc{.title = "Antwika"}, DemoFixture::logo(), 3);
}

TEST(DemoLoopTest, Run_OpensAndClosesTheWindowEvenWithNoFrames)
{
    DemoFixture fixture;
    fixture.expectOneWindow(true);

    EXPECT_CALL(fixture.backend, pollEvent()).Times(0);
    EXPECT_CALL(fixture.renderer, present()).Times(0);

    const DemoScene scene;
    FakeInputBackend input;
    antwika::time::fakes::FakeSleeper sleeper;
    DemoLoop loop(
        fixture.backend, input, scene, sleeper, kTestFramePeriod);

    EXPECT_CALL(*fixture.window, close()).Times(1);

    loop.run(WindowDesc{.title = "Antwika"}, DemoFixture::logo(), 0);
}

TEST(DemoLoopTest, Run_CountsAPressOnTheCountingButton)
{
    DemoFixture fixture;
    fixture.expectOneWindow(true);

    ON_CALL(fixture.backend, pollEvent())
        .WillByDefault(Return(std::nullopt));

    const DemoScene scene;
    FakeInputBackend input(clickAt(positionOn(widgets::kCount)));
    antwika::time::fakes::FakeSleeper sleeper;
    DemoLoop loop(
        fixture.backend, input, scene, sleeper, kTestFramePeriod);

    loop.run(WindowDesc{.title = "Antwika"}, DemoFixture::logo(), 2);

    EXPECT_EQ(1U, loop.clicks());
}

TEST(DemoLoopTest, Run_PutsTheCountBackOnAPressOnTheResetButton)
{
    DemoFixture fixture;
    fixture.expectOneWindow(true);

    ON_CALL(fixture.backend, pollEvent())
        .WillByDefault(Return(std::nullopt));

    const DemoScene scene;
    FakeInputBackend input(std::vector<std::vector<InputEvent>>{
        clickAt(positionOn(widgets::kCount)),
        clickAt(positionOn(widgets::kReset)),
        clickAt(positionOn(widgets::kCount))});
    antwika::time::fakes::FakeSleeper sleeper;
    DemoLoop loop(
        fixture.backend, input, scene, sleeper, kTestFramePeriod);

    loop.run(WindowDesc{.title = "Antwika"}, DemoFixture::logo(), 3);

    EXPECT_EQ(1U, loop.clicks());
}

TEST(DemoLoopTest, Run_IgnoresAPressThatLandsOnNoButton)
{
    DemoFixture fixture;
    fixture.expectOneWindow(true);

    ON_CALL(fixture.backend, pollEvent())
        .WillByDefault(Return(std::nullopt));

    const DemoScene scene;
    FakeInputBackend input(clickAt(Position{
        .x = static_cast<std::int32_t>(kCanvas.width) - 1,
        .y = static_cast<std::int32_t>(kCanvas.height) - 1}));
    antwika::time::fakes::FakeSleeper sleeper;
    DemoLoop loop(
        fixture.backend, input, scene, sleeper, kTestFramePeriod);

    loop.run(WindowDesc{.title = "Antwika"}, DemoFixture::logo(), 1);

    EXPECT_EQ(0U, loop.clicks());
}

TEST(DemoLoopTest, Run_TakesAPressWithNoMovementBeforeIt)
{
    DemoFixture fixture;
    fixture.expectOneWindow(true);

    ON_CALL(fixture.backend, pollEvent())
        .WillByDefault(Return(std::nullopt));

    const DemoScene scene;
    FakeInputBackend input(std::vector<InputEvent>{PointerButtonPressed{
        .button = MouseButton::Left,
        .position = positionOn(widgets::kCount)}});
    antwika::time::fakes::FakeSleeper sleeper;
    DemoLoop loop(
        fixture.backend, input, scene, sleeper, kTestFramePeriod);

    loop.run(WindowDesc{.title = "Antwika"}, DemoFixture::logo(), 1);

    EXPECT_EQ(1U, loop.clicks());
}

TEST(DemoLoopTest, Run_CountsNothingForAReleaseOnAButton)
{
    DemoFixture fixture;
    fixture.expectOneWindow(true);

    ON_CALL(fixture.backend, pollEvent())
        .WillByDefault(Return(std::nullopt));

    const DemoScene scene;
    FakeInputBackend input(std::vector<InputEvent>{PointerButtonReleased{
        .button = MouseButton::Left,
        .position = positionOn(widgets::kCount)}});
    antwika::time::fakes::FakeSleeper sleeper;
    DemoLoop loop(
        fixture.backend, input, scene, sleeper, kTestFramePeriod);

    loop.run(WindowDesc{.title = "Antwika"}, DemoFixture::logo(), 1);

    EXPECT_EQ(0U, loop.clicks());
}

TEST(DemoLoopTest, Run_LightsAButtonThePointerMerelyMovedOnto)
{
    DemoFixture fixture;
    fixture.expectOneWindow(true);

    ON_CALL(fixture.backend, pollEvent())
        .WillByDefault(Return(std::nullopt));

    EXPECT_CALL(fixture.renderer, drawRect(::testing::_, ::testing::_))
        .Times(::testing::AnyNumber());

    EXPECT_CALL(
        fixture.renderer,
        drawRect(::testing::_, antwika::ui::Theme{}.buttonHovered))
        .Times(::testing::AtLeast(1));

    const DemoScene scene;
    FakeInputBackend input(std::vector<InputEvent>{
        PointerMoved{.position = positionOn(widgets::kCount)}});
    antwika::time::fakes::FakeSleeper sleeper;
    DemoLoop loop(
        fixture.backend, input, scene, sleeper, kTestFramePeriod);

    loop.run(WindowDesc{.title = "Antwika"}, DemoFixture::logo(), 1);

    EXPECT_EQ(0U, loop.clicks());
}

TEST(DemoLoopTest, Run_DrawsNoHighlightBeforeThePointerHasBeenSeen)
{
    DemoFixture fixture;
    fixture.expectOneWindow(true);

    ON_CALL(fixture.backend, pollEvent())
        .WillByDefault(Return(std::nullopt));

    EXPECT_CALL(fixture.renderer, drawRect(::testing::_, ::testing::_))
        .Times(::testing::AnyNumber());

    EXPECT_CALL(
        fixture.renderer,
        drawRect(::testing::_, antwika::ui::Theme{}.buttonHovered))
        .Times(0);

    const DemoScene scene;
    FakeInputBackend input;
    antwika::time::fakes::FakeSleeper sleeper;
    DemoLoop loop(
        fixture.backend, input, scene, sleeper, kTestFramePeriod);

    loop.run(WindowDesc{.title = "Antwika"}, DemoFixture::logo(), 1);
}

TEST(DemoLoopTest, Run_LeavesThePointerNowhereForAKeyPress)
{
    DemoFixture fixture;
    fixture.expectOneWindow(true);

    ON_CALL(fixture.backend, pollEvent())
        .WillByDefault(Return(std::nullopt));

    const DemoScene scene;
    FakeInputBackend input(
        std::vector<InputEvent>{KeyPressed{.key = antwika::input::Key::A}});
    antwika::time::fakes::FakeSleeper sleeper;
    DemoLoop loop(
        fixture.backend, input, scene, sleeper, kTestFramePeriod);

    loop.run(WindowDesc{.title = "Antwika"}, DemoFixture::logo(), 1);

    EXPECT_EQ(0U, loop.clicks());
}
