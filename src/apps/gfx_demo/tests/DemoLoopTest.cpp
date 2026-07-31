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
    // The size every test lays its widgets out against.
    constexpr Size kCanvas{.width = 700, .height = 400};

    // The window a run draws into, which is what a click lands in.
    // Where a widget sits inside it is the layout's business.
    // So a test looks for a pixel that hits the one it means.
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

        // A silent fallback presses on nothing at all.
        // A test asserting a count of zero would then pass.
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
            ON_CALL(*window, size()).WillByDefault(Return(kCanvas));

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
    FakeInputBackend input;
    DemoLoop loop(fixture.backend, input, scene);

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
    DemoLoop loop(fixture.backend, input, scene);

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
    FakeInputBackend input;
    DemoLoop loop(fixture.backend, input, scene);

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
    DemoLoop loop(fixture.backend, input, scene);

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
    FakeInputBackend input;
    DemoLoop loop(fixture.backend, input, scene);

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
    FakeInputBackend input;
    DemoLoop loop(fixture.backend, input, scene);

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
    DemoLoop loop(fixture.backend, input, scene);

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
    DemoLoop loop(fixture.backend, input, scene);

    loop.run(WindowDesc{.title = "Antwika"}, DemoFixture::logo(), 2);

    // One press, not one per frame it stays down for.
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
    DemoLoop loop(fixture.backend, input, scene);

    loop.run(WindowDesc{.title = "Antwika"}, DemoFixture::logo(), 3);

    // Counting again after the reset is what tells the two apart.
    // A reset that did nothing would leave two.
    // A count that did nothing would leave none.
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
    DemoLoop loop(fixture.backend, input, scene);

    loop.run(WindowDesc{.title = "Antwika"}, DemoFixture::logo(), 1);

    EXPECT_EQ(0U, loop.clicks());
}

// A press carries its own position.
// So it can be the first thing that ever says where the pointer is.
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
    DemoLoop loop(fixture.backend, input, scene);

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
    DemoLoop loop(fixture.backend, input, scene);

    loop.run(WindowDesc{.title = "Antwika"}, DemoFixture::logo(), 1);

    // A widget activates on the press, and a release is not one.
    EXPECT_EQ(0U, loop.clicks());
}

// The pointer only moves, and nothing is pressed.
// A recording under input::IdleMotionSource would hold none of it.
// The button lights up all the same, off the hint channel.
TEST(DemoLoopTest, Run_LightsAButtonThePointerMerelyMovedOnto)
{
    DemoFixture fixture;
    fixture.expectOneWindow(true);

    ON_CALL(fixture.backend, pollEvent())
        .WillByDefault(Return(std::nullopt));

    // The scene fills bars and panels too, and none of those is meant.
    EXPECT_CALL(fixture.renderer, drawRect(::testing::_, ::testing::_))
        .Times(::testing::AnyNumber());

    // The one assertion: the hovered fill reaches the renderer.
    // Without the pass the gated pointer is nowhere and both are idle.
    EXPECT_CALL(
        fixture.renderer,
        drawRect(::testing::_, antwika::ui::Theme{}.buttonHovered))
        .Times(::testing::AtLeast(1));

    const DemoScene scene;
    FakeInputBackend input(std::vector<InputEvent>{
        PointerMoved{.position = positionOn(widgets::kCount)}});
    DemoLoop loop(fixture.backend, input, scene);

    loop.run(WindowDesc{.title = "Antwika"}, DemoFixture::logo(), 1);

    // And it stayed appearance: a movement activates nothing.
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

    // Nothing has said where the pointer is, so nothing is hovered.
    // An origin would light whichever widget sits in the corner.
    EXPECT_CALL(
        fixture.renderer,
        drawRect(::testing::_, antwika::ui::Theme{}.buttonHovered))
        .Times(0);

    const DemoScene scene;
    FakeInputBackend input;
    DemoLoop loop(fixture.backend, input, scene);

    loop.run(WindowDesc{.title = "Antwika"}, DemoFixture::logo(), 1);
}

// A key says nothing about where the pointer is.
// So one arriving first must not put it in the window's corner.
TEST(DemoLoopTest, Run_LeavesThePointerNowhereForAKeyPress)
{
    DemoFixture fixture;
    fixture.expectOneWindow(true);

    ON_CALL(fixture.backend, pollEvent())
        .WillByDefault(Return(std::nullopt));

    const DemoScene scene;
    FakeInputBackend input(
        std::vector<InputEvent>{KeyPressed{.key = antwika::input::Key::A}});
    DemoLoop loop(fixture.backend, input, scene);

    loop.run(WindowDesc{.title = "Antwika"}, DemoFixture::logo(), 1);

    EXPECT_EQ(0U, loop.clicks());
}
